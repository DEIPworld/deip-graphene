#include <deip/chain/dbs_review.hpp>
#include <deip/chain/database.hpp>

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

const review_object& dbs_review::create(const research_content_id_type &research_content_id,
                                        const string &content,
                                        bool is_positive,
                                        const account_name_type &author,
                                        const std::set<discipline_id_type>& disciplines)
{
    const auto& new_review = db_impl().create<review_object>([&](review_object& r) {

        auto now = db_impl().head_block_time();

        r.research_content_id = research_content_id;
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

} //namespace chain
} //namespace deip