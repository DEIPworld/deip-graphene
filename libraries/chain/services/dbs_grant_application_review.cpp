#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_grant_application_review.hpp>
#include <tuple>

namespace deip {
namespace chain {

dbs_grant_application_review::dbs_grant_application_review(database& db)
    : _base_type(db)
{
}

const grant_application_review_object& dbs_grant_application_review::create(const grant_application_id_type& grant_application_id,
                                                          const string& content,
                                                          bool is_positive,
                                                          const account_name_type& author,
                                                          const std::set<discipline_id_type>& disciplines)
{
    const auto& new_review = db_impl().create<grant_application_review_object>([&](grant_application_review_object& r) {
        const auto now = db_impl().head_block_time();

        r.grant_application_id = grant_application_id;
        fc::from_string(r.content, content);
        r.author = author;
        r.is_positive = is_positive;
        r.created_at = now;
        r.disciplines.insert(disciplines.begin(), disciplines.end());
    });

    return new_review;
}

const grant_application_review_object& dbs_grant_application_review::get(const grant_application_review_id_type& id) const
{
    try
    {
        return db_impl().get<grant_application_review_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

dbs_grant_application_review::grant_application_review_refs_type dbs_grant_application_review::get_grant_application_reviews(const grant_application_id_type& grant_application_id) const
{
    grant_application_review_refs_type ret;

    auto it_pair = db_impl().get_index<grant_application_review_index>().indicies().get<by_grant_application>().equal_range(grant_application_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_grant_application_review::grant_application_review_refs_type dbs_grant_application_review::get_author_reviews(const account_name_type& author) const
{
    grant_application_review_refs_type ret;

    auto it_pair = db_impl().get_index<grant_application_review_index>().indicies().get<by_author>().equal_range(author);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

} // namespace chain
} // namespace deip