#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_expertise_stats.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_vote.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_review::dbs_review(database &db)
        : _base_type(db)
{
}

dbs_review::review_refs_type dbs_review::get_research_content_reviews(const research_content_id_type &research_content_id) const
{
    review_refs_type ret;

    auto it_pair = db_impl().get_index<review_index>().indicies().get<by_research_content>().equal_range(research_content_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_review::review_refs_type dbs_review::get_grant_application_reviews(const grant_application_id_type& grant_application_id) const
{
    review_refs_type ret;

    auto it_pair = db_impl().get_index<review_index>().indicies().get<by_grant_application>().equal_range(grant_application_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_review::review_refs_type dbs_review::get_author_reviews(const account_name_type &author) const
{
    review_refs_type ret;

    auto it_pair = db_impl().get_index<review_index>().indicies().get<by_author>().equal_range(author);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const review_object& dbs_review::create(const int64_t& object_id,
                                        const bool is_grant_application,
                                        const string &content,
                                        bool is_positive,
                                        const account_name_type &author,
                                        const std::set<discipline_id_type>& disciplines)
{
    const auto& new_review = db_impl().create<review_object>([&](review_object& r) {

        auto now = db_impl().head_block_time();

        if (is_grant_application)
            r.grant_application_id = object_id;
        else
            r.research_content_id = object_id;
        r.is_grant_application = is_grant_application;
        fc::from_string(r.content, content);
        r.author = author;
        r.is_positive = is_positive;
        r.created_at = now;
        r.disciplines.insert(disciplines.begin(), disciplines.end());
    });

    return new_review;
}

const review_object& dbs_review::get(const review_id_type &id)
{
    try
    {
        return db_impl().get<review_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

// TODO: Split this method into separate methods for content and grant application
void dbs_review::make_review_execution(const int64_t& object_id,
                                       const research_id_type& research_id,
                                       const bool is_grant_application,
                                       const account_name_type& author,
                                       const uint16_t& weight,
                                       const bool is_positive,
                                       const std::string& content)
{
    dbs_research_group& research_group_service = db_impl().obtain_service<dbs_research_group>();
    dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    dbs_research_content& research_content_service = db_impl().obtain_service<dbs_research_content>();
    dbs_research_discipline_relation& research_discipline_service = db_impl().obtain_service<dbs_research_discipline_relation>();
    dbs_expert_token& expertise_token_service = db_impl().obtain_service<dbs_expert_token>();
    dbs_expertise_stats& expertise_stats_service = db_impl().obtain_service<dbs_expertise_stats>();
    dbs_vote& votes_service = db_impl().obtain_service<dbs_vote>();
    dbs_discipline& discipline_service = db_impl().obtain_service<dbs_discipline>();

    auto& research = research_service.get_research(research_id);
    auto reseach_group_tokens = research_group_service.get_research_group_tokens(research.research_group_id);

    for (auto& reseach_group_token : reseach_group_tokens)
        FC_ASSERT(reseach_group_token.get().owner != author, "You cannot review your own content");

    auto expertise_tokens = expertise_token_service.get_expert_tokens_by_account_name(author);
    auto research_discipline_relations = research_discipline_service.get_research_discipline_relations_by_research(research_id);
    std::map<discipline_id_type, share_type> review_disciplines_with_weight;
    std::set<discipline_id_type> review_disciplines;
    std::set<discipline_id_type> research_disciplines_ids;
    for (auto rdr : research_discipline_relations) {
        research_disciplines_ids.insert(rdr.get().discipline_id);
    }

    for (auto expert_token : expertise_tokens)
    {
        auto& token = expert_token.get();
        if (research_disciplines_ids.find(token.discipline_id) != research_disciplines_ids.end())
        {
            const int64_t elapsed_seconds   = (db_impl().head_block_time() - token.last_vote_time).to_seconds();

            const int64_t regenerated_power = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
            const int64_t current_power = std::min(int64_t(token.voting_power + regenerated_power), int64_t(DEIP_100_PERCENT));
            FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

            const int64_t used_power = (DEIP_REVIEW_REQUIRED_POWER_PERCENT * weight) / DEIP_100_PERCENT;

            FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

            const uint64_t abs_used_tokens = ((uint128_t(token.amount.value) * current_power) / (DEIP_100_PERCENT)).to_uint64();

            db_impl()._temporary_public_impl().modify(token, [&](expert_token_object& t) {
                t.voting_power = current_power - used_power;
                t.last_vote_time = db_impl().head_block_time();
            });
            review_disciplines_with_weight.insert(std::make_pair(token.discipline_id, abs_used_tokens));
            review_disciplines.insert(token.discipline_id);
        }
    }

    FC_ASSERT(review_disciplines.size() != 0, "Reviewer does not have enough expertise to make review.");

    auto& review = create(object_id, is_grant_application, content, is_positive, author, review_disciplines);

    for (auto& review_discipline : review_disciplines) {
        auto &token = expertise_token_service.get_expert_token_by_account_and_discipline(author, review_discipline);

        auto used_expertise = review_disciplines_with_weight.at(token.discipline_id);

        db_impl()._temporary_public_impl().modify(review, [&](review_object& r) {
            r.expertise_amounts_used[token.discipline_id] = used_expertise;
            r.weights_per_discipline[token.discipline_id] = used_expertise;
            r.weight_modifiers[token.discipline_id] = 1;
        });

        if (!is_grant_application) {
            auto& research_content = research_content_service.get(object_id);
            if (votes_service.is_exists_by_content_and_discipline(object_id, token.discipline_id)) {
                auto &total_votes = votes_service.get_total_votes_by_content_and_discipline(object_id,
                                                                                            token.discipline_id);

                db_impl()._temporary_public_impl().modify(total_votes, [&](total_votes_object &tv) {
                    tv.total_weight += used_expertise;
                });

            } else {
                db_impl()._temporary_public_impl().create<total_votes_object>([&](total_votes_object &tv) {
                    tv.discipline_id = token.discipline_id;
                    tv.research_content_id = research_content.id;
                    tv.research_id = research_id;
                    tv.total_weight = used_expertise;
                    tv.content_type = research_content.type;
                });
            }

            db_impl()._temporary_public_impl().modify(research_content, [&](research_content_object& rc_o) {
                rc_o.eci_per_discipline[review_discipline] += review.get_evaluation(token.discipline_id);
            });
        }

        auto& discipline = discipline_service.get_discipline(token.discipline_id);
        db_impl()._temporary_public_impl().modify(discipline, [&](discipline_object& d) {
            d.total_active_weight += used_expertise;
        });

        db_impl()._temporary_public_impl().modify(research, [&](research_object& r_o) {
            if (review.is_positive)
                r_o.number_of_positive_reviews++;
            else
                r_o.number_of_negative_reviews++;
        });

        if (!is_grant_application)
            research_service.calculate_eci(research_id);
        expertise_stats_service.update_used_expertise(used_expertise);
    }
}


} //namespace chain
} //namespace deip