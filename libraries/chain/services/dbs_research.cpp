#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>

namespace deip{
namespace chain{

dbs_research::dbs_research(database &db) : _base_type(db)
{
}

const research_object& dbs_research::create_research(const research_group_object& research_group,
                                                     const external_id_type& external_id,
                                                     const string& description,
                                                     const std::set<discipline_id_type>& disciplines,
                                                     const optional<percent>& review_share,
                                                     const optional<percent>& compensation_share,
                                                     const bool& is_private,
                                                     const bool& is_finished,
                                                     const flat_set<account_name_type>& members,
                                                     const time_point_sec& created_at,
                                                     const bool& is_default)
{
    auto& research_disciplines_service = db_impl().obtain_service<dbs_research_discipline_relation>();
    auto& dgp_service = db_impl().obtain_service<dbs_dynamic_global_properties>();

    const auto& research = db_impl().create<research_object>([&](research_object& r_o) {
        r_o.research_group_id = research_group.id;
        r_o.research_group = research_group.account;
        r_o.external_id = external_id;
        fc::from_string(r_o.description, description);
        r_o.review_share = review_share;
        r_o.compensation_share = compensation_share;
        r_o.is_private = is_private;
        r_o.is_finished = is_finished;
        r_o.is_default = is_default;
        r_o.members = members;
        r_o.created_at = created_at;
        r_o.last_update_time = created_at;
        r_o.review_share_last_update = created_at;
    });

    for (auto& discipline_id : disciplines)
    {
        research_disciplines_service.create_research_relation(research.id, discipline_id);
    }

    dgp_service.create_recent_entity(external_id);

    return research;
}

const research_object& dbs_research::update_research(const research_object& research,
                                                     const string& description,
                                                     const bool& is_private,
                                                     const optional<percent>& review_share,
                                                     const optional<percent>& compensation_share,
                                                     const flat_set<account_name_type>& members)
{

    const auto& block_time = db_impl().head_block_time();
    const auto updated_members = members;
    db_impl().modify(research, [&](research_object& r_o) {
        fc::from_string(r_o.description, description);
        r_o.is_private = is_private;
        if (r_o.review_share != review_share)
        {
            r_o.review_share_last_update = block_time;
        }
        r_o.review_share = review_share;
        r_o.compensation_share = compensation_share;
        r_o.last_update_time = block_time;
        r_o.members.clear();
        r_o.members.insert(updated_members.begin(), updated_members.end());
    });

    return research;
}

const dbs_research::research_refs_type dbs_research::get_researches_by_research_group(const research_group_id_type& research_group_id) const
{
    research_refs_type ret;

    const auto& idx = db_impl()
      .get_index<research_index>()
      .indicies()
      .get<by_research_group_id>();

    auto it_pair = idx.equal_range(research_group_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const dbs_research::research_refs_type dbs_research::get_researches_by_research_group(const account_name_type& research_group) const
{
    research_refs_type ret;

    const auto& idx = db_impl()
      .get_index<research_index>()
      .indicies()
      .get<by_research_group>();

    auto it_pair = idx.equal_range(research_group);

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
    try { return db_impl().get<research_object>(id); }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_research::research_optional_ref_type dbs_research::get_research_if_exists(const research_id_type& id) const
{
    research_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr !=idx.end())
    {
        result = *itr;
    }

    return result;
}

const research_object& dbs_research::get_research(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<research_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    FC_ASSERT(itr != idx.end(), "Research with external_id ${1} does not exist", ("1", external_id));
    return *itr;
}

const dbs_research::research_optional_ref_type dbs_research::get_research_if_exists(const external_id_type& external_id) const
{
    research_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    if (itr !=idx.end())
    {
        result = *itr;
    }

    return result;
}

void dbs_research::check_research_existence(const research_id_type& id) const
{
    auto research = db_impl().find<research_object, by_id>(id);
    FC_ASSERT(research != nullptr, "Research with id \"${1}\" must exist.", ("1", id));
}

const std::map<discipline_id_type, share_type> dbs_research::get_eci_evaluation(const research_id_type& research_id) const
{
    const dbs_research_content& research_content_service = db_impl().obtain_service<dbs_research_content>();
    const dbs_review& review_service = db_impl().obtain_service<dbs_review>();
    const dbs_research_discipline_relation& research_discipline_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();
    const dbs_review_vote& review_votes_service = db_impl().obtain_service<dbs_review_vote>();

    const research_object& research = get_research(research_id);
    const auto& research_contents = research_content_service.get_research_content_by_research_id(research.id);
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
    for (const research_content_object& research_content : research_contents)
    {
        if (research.is_finished && research_content.id == final_result->id) {
            continue;
        }

        const auto& milestone_reviews = review_service.get_reviews_by_research_content(research_content.id);

        for (const review_object& milestone_review : milestone_reviews)
        {
            if (final_result_reviewers.find(milestone_review.author) != final_result_reviewers.end()) {
                continue;
            }

            for (discipline_id_type discipline_id: milestone_review.disciplines)
            {
                const auto& milestone_review_votes = review_votes_service.get_review_votes(milestone_review.id);
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
    for (const research_discipline_relation_object& relation : research_discipline_relations)
    {
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

        const share_type Sdfr = Vdp + Sdp;

        research_eci_by_discipline[discipline_id] = Sdfr;
    }

    return research_eci_by_discipline;
}

const research_object& dbs_research::update_eci_evaluation(const research_id_type& research_id)
{
    const dbs_research_discipline_relation& research_discipline_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();

    const auto& research = get_research(research_id);
    const auto& eci_evaluation = get_eci_evaluation(research_id);

    db_impl().modify(research, [&](research_object& r_o) {
        for (const auto& entry : eci_evaluation)
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

const bool dbs_research::research_exists(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<research_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    return itr != idx.end();
}

const dbs_research::research_refs_type dbs_research::lookup_researches(const research_id_type& lower_bound, uint32_t limit) const
{
    research_refs_type result;

    const auto& idx = db_impl()
      .get_index<research_index>()
      .indicies()
      .get<by_id>();

    for (auto itr = idx.lower_bound(lower_bound); limit-- && itr != idx.end(); ++itr)
    {
        result.push_back(std::cref(*itr));
    }

    return result;
}

const dbs_research::research_refs_type dbs_research::get_researches_by_member(const account_name_type& member) const
{
    const auto& research_groups_service = db_impl().obtain_service<dbs_research_group>();

    research_refs_type ret;

    const auto& rgt_list = research_groups_service.get_research_group_tokens_by_member(member);
    for (const research_group_token_object& rgt : rgt_list)
    {
        const auto& research_list = get_researches_by_research_group(rgt.research_group_id);
        for (const research_object& research : research_list)
        {
            if (research.members.find(member) != research.members.end())
            {
                ret.push_back(research);
            }
        }
    }

    return ret;
}

}
}