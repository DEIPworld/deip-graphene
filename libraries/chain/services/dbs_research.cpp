#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>

namespace deip{
namespace chain{

dbs_research::dbs_research(database &db) : _base_type(db)
{
}

const research_object& dbs_research::create(const string &title, const string &abstract, const string &permlink,
                                            const research_group_id_type &research_group_id, const uint16_t review_share_in_percent,
                                            const uint16_t dropout_compensation_in_percent, const bool is_private)
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
        r.is_private = is_private;
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

const std::map<discipline_id_type, share_type> dbs_research::get_eci_evaluation(const research_id_type& research_id) const
{
    const dbs_research_content& research_content_service = db_impl().obtain_service<dbs_research_content>();
    const dbs_review& review_service = db_impl().obtain_service<dbs_review>();
    const dbs_research_discipline_relation& research_discipline_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();
    const dbs_review_vote& review_votes_service = db_impl().obtain_service<dbs_review_vote>();

    const research_object& research = get_research(research_id);
    const auto& research_contents = research_content_service.get_by_research_id(research.id);
    const auto& research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);

    const fc::optional<research_content_object> final_result = research.is_finished
        ? research_content_service.get_by_research_and_type(research.id, research_content_type::final_result)[0]
        : fc::optional<research_content_object>();

    const std::vector<std::reference_wrapper<const review_object>>& final_result_reviews = research.is_finished
        ? review_service.get_reviews_by_research_content(final_result->id)
        : std::vector<std::reference_wrapper<const review_object>>();

    const std::set<account_name_type> final_result_reviewers = research.is_finished
        ? std::accumulate(final_result_reviews.begin(), final_result_reviews.end(), std::set<account_name_type>(),
            [](std::set<account_name_type> acc, std::reference_wrapper<const review_object> rw_wrap) {
                const review_object& rw = rw_wrap.get();
                acc.insert(rw.author);
                return acc;
            })
        : std::set<account_name_type>();

    const std::map<discipline_id_type, share_type> final_result_weight = research.is_finished
        ? research_content_service.get_eci_evaluation(final_result->id)
        : std::map<discipline_id_type, share_type>();

    std::map<discipline_id_type, std::map<account_name_type, std::pair<share_type, share_type>>> max_and_min_reviewer_weight_by_discipline;
    for (auto& wrap : research_contents)
    {
        const research_content_object& research_content = wrap.get();
        if (research.is_finished && research_content.id == final_result->id) {
            continue;
        }

        const auto& milestone_reviews = review_service.get_reviews_by_research_content(research_content.id);

        for (auto& rw_wrap: milestone_reviews)
        {
            const review_object& milestone_review = rw_wrap.get();
            if (final_result_reviewers.find(milestone_review.author) != final_result_reviewers.end()) {
                continue;
            }

            for (discipline_id_type discipline_id: milestone_review.disciplines)
            {
                auto milestone_review_votes = review_votes_service.get_review_votes(milestone_review.id);
                const double milestone_review_votes_count = (double)std::count_if(
                    milestone_review_votes.begin(), milestone_review_votes.end(),
                    [&](const review_vote_object& rw_vote) {
                        return rw_vote.review_id == milestone_review.id && rw_vote.discipline_id == discipline_id;
                    });

                if (max_and_min_reviewer_weight_by_discipline.find(discipline_id) == max_and_min_reviewer_weight_by_discipline.end()) {
                    std::map<account_name_type, std::pair<share_type, share_type>> author_max_and_min_weights;
                    max_and_min_reviewer_weight_by_discipline[discipline_id] = author_max_and_min_weights;
                }

                auto& author_max_and_min_weights = max_and_min_reviewer_weight_by_discipline[discipline_id];
                if (author_max_and_min_weights.find(milestone_review.author) == author_max_and_min_weights.end()) {
                    std::pair<share_type, share_type> max_and_min_weights;
                    author_max_and_min_weights[milestone_review.author] = max_and_min_weights;
                }

                auto& max_and_min_weights = author_max_and_min_weights[milestone_review.author];
                const share_type expertise_used = milestone_review.expertise_tokens_amount_by_discipline.at(discipline_id);
                if (milestone_review.is_positive) {
                    max_and_min_weights.first = max_and_min_weights.first < expertise_used
                        ? expertise_used + (milestone_review_votes_count * DEIP_CURATOR_INFLUENCE_BONUS)
                        : max_and_min_weights.first;
                } else {
                    max_and_min_weights.second = max_and_min_weights.second < expertise_used
                        ? expertise_used + (milestone_review_votes_count * DEIP_CURATOR_INFLUENCE_BONUS)
                        : max_and_min_weights.second;
                }
            }
        }
    }

    std::map<discipline_id_type, share_type> research_eci_by_discipline;
    for (auto& wrap: research_discipline_relations)
    {
        const research_discipline_relation_object& relation = wrap.get();
        const discipline_id_type discipline_id = relation.discipline_id;

        const auto authors_weights = max_and_min_reviewer_weight_by_discipline.find(discipline_id) != max_and_min_reviewer_weight_by_discipline.end()
            ? max_and_min_reviewer_weight_by_discipline[discipline_id]
            : std::map<account_name_type, std::pair<share_type, share_type>>();

        const share_type Vdp = std::accumulate(authors_weights.begin(), authors_weights.end(), share_type(0),
            [=](share_type acc, const std::pair<account_name_type, std::pair<share_type, share_type>>& entry) {
                const share_type Ek_max = entry.second.first;
                const share_type Ek_min = entry.second.second;
                return acc + (Ek_max - Ek_min);
            });

        const share_type Sdp = final_result_weight.find(discipline_id) != final_result_weight.end()
            ? final_result_weight.at(discipline_id)
            : share_type(0);

        /*

        Original Formula

        const share_type Sdfr = Vdp + Sdp;

        */

        const share_type Sdfr = Vdp + Sdp;

        research_eci_by_discipline[discipline_id] = Sdfr > 0 ? Sdfr : 0;
    }

    return research_eci_by_discipline;
}

const research_object& dbs_research::update_eci_evaluation(const research_id_type& research_id)
{
    const dbs_research_discipline_relation& research_discipline_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();

    const auto& research = get_research(research_id);
    const auto& eci_evaluation = get_eci_evaluation(research_id);

    db_impl().modify(research, [&](research_object& r_o) {
        for (auto& entry : eci_evaluation)
        {
            r_o.eci_per_discipline[entry.first] = entry.second;
        }
    });

    for (auto& entry : eci_evaluation)
    {
        const auto& relation = research_discipline_relation_service.get_research_discipline_relation_by_research_and_discipline(research_id, entry.first);
        db_impl().modify(relation, [&](research_discipline_relation_object& rdr_o) {
            rdr_o.research_eci = entry.second;
        });
    }

    return research;
}

const bool dbs_research::research_exists(const research_id_type& research_id) const
{
    const auto& idx = db_impl()
      .get_index<research_index>()
      .indices()
      .get<by_id>();

    auto itr = idx.find(research_id);
    return itr != idx.end();
}

}
}