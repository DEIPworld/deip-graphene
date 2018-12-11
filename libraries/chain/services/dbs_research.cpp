#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/database/database.hpp>

namespace deip{
namespace chain{

dbs_research::dbs_research(database &db) : _base_type(db)
{
}

const research_object& dbs_research::create(const string &title, const string &abstract, const string &permlink,
                                            const research_group_id_type &research_group_id, const uint16_t review_share_in_percent,
                                            const uint16_t dropout_compensation_in_percent)
{
    const auto& new_research = db_impl().create<research_object>([&](research_object& r) {
        fc::from_string(r.title, title);
        fc::from_string(r.abstract, abstract);
        fc::from_string(r.permlink, permlink);
        r.research_group_id = research_group_id;
        r.review_share_in_percent = review_share_in_percent;
        r.dropout_compensation_in_percent = dropout_compensation_in_percent;
        r.is_finished = false;
        r.owned_tokens = DEIP_100_PERCENT;
        r.created_at = db_impl().head_block_time();
        r.last_update_time = db_impl().head_block_time();
        r.review_share_in_percent_last_update = db_impl().head_block_time();
    });

    return new_research;
}

dbs_research::research_refs_type dbs_research::get_researches() const
{
    research_refs_type ret;

    const auto& idx = db_impl().get_index<research_index>().indicies().get<by_id>();
    auto it = idx.lower_bound(0);
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research::research_refs_type dbs_research::get_researches_by_research_group(const research_group_id_type& research_group_id) const
{
    research_refs_type ret;

    auto it_pair
        = db_impl().get_index<research_index>().indicies().get<by_research_group>().equal_range(research_group_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_object& dbs_research::get_research(const research_id_type& id) const
{
    try {
        return db_impl().get<research_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const research_object& dbs_research::get_research_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const
{
    const auto& idx = db_impl().get_index<research_index>().indices().get<by_permlink>();
    auto itr = idx.find(std::make_tuple(research_group_id, permlink));
    FC_ASSERT(itr != idx.end(), "Research by permlink ${p} is not found", ("p", permlink));
    return *itr;
}

void dbs_research::check_research_existence(const research_id_type& id) const
{
    auto research = db_impl().find<research_object, by_id>(id);
    FC_ASSERT(research != nullptr, "Research with id \"${1}\" must exist.", ("1", id));
}

void dbs_research::decrease_owned_tokens(const research_object& research, const share_type delta)
{
    FC_ASSERT((research.owned_tokens - delta > 0), "Cannot update research owned tokens (result amount < 0)");
    db_impl().modify(research, [&](research_object& r_o) { r_o.owned_tokens -= delta; });
}

void dbs_research::increase_owned_tokens(const research_object& research, const share_type delta)
{
    FC_ASSERT(delta >= 0, "Cannot update research owned tokens (delta < 0)");
    db_impl().modify(research, [&](research_object& r_o) { r_o.owned_tokens += delta; });
}

void dbs_research::change_research_review_share_percent(const research_id_type& research_id,
                                                        const uint16_t review_share_in_percent)
{
    check_research_existence(research_id);
    const research_object& research = get_research(research_id);
    db_impl().modify(research, [&](research_object& r) {
        r.review_share_in_percent = review_share_in_percent;
        r.review_share_in_percent_last_update = db_impl().head_block_time();
    });
}

void dbs_research::calculate_eci(const research_id_type& research_id)
{
    check_research_existence(research_id);

    dbs_research_content& research_content_service = db_impl().obtain_service<dbs_research_content>();
    dbs_review& review_service = db_impl().obtain_service<dbs_review>();
    auto research_contents = research_content_service.get_by_research_id(research_id);

    std::map<std::pair<account_name_type, discipline_id_type>, share_type> positive_weights;
    std::map<std::pair<account_name_type, discipline_id_type>, share_type> negative_weights;

    for (auto& cnt : research_contents)
    {
        auto& content = cnt.get();
        auto reviews = review_service.get_research_content_reviews(content.id);
        for (auto& rw : reviews)
        {
            auto& review = rw.get();
            auto& weights = review.is_positive ? positive_weights : negative_weights;
            for (auto& review_discipline : review.disciplines)
            {
                auto current_weight = weights.find(std::make_pair(review.author, review_discipline));
                if (current_weight != weights.end())
                    current_weight->second = std::max(abs(current_weight->second.value), abs(review.get_evaluation(review_discipline).value));
                else
                    weights[std::make_pair(review.author,review_discipline)] = abs(review.get_evaluation(review_discipline).value);
            }
        }
    }

    std::map<std::pair<account_name_type, discipline_id_type>, share_type> total_weights = positive_weights;
    for (auto it = negative_weights.begin(); it != negative_weights.end(); ++it) {
        total_weights[it->first] -= it->second;
    }

        auto& research = db_impl().get<research_object>(research_id);

    std::map<discipline_id_type, share_type> discipline_total_weights;
    for (auto it = total_weights.begin(); it != total_weights.end(); ++it)
    {
        auto& discipline_id = it->first.second;
        auto& weight = it->second;
        discipline_total_weights[discipline_id] += weight;
    }

    auto& rd_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();

    for (auto it = discipline_total_weights.begin(); it != discipline_total_weights.end(); ++it) {
        auto discipline_id = it->first;
        auto weight = it->second;
        db_impl().modify(research,
                         [&](research_object &r_o) { r_o.eci_per_discipline[discipline_id].value = weight.value; });
        auto& rd_relation = rd_relation_service.get_research_discipline_relations_by_research_and_discipline(research_id, discipline_id);
        db_impl().modify(rd_relation,
                         [&](research_discipline_relation_object &rdr_o) { rdr_o.resrearch_eci = research.eci_per_discipline.at(discipline_id); });
    }
}

}
}