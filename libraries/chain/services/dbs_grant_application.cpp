#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>

namespace deip {
namespace chain {

dbs_grant_application::dbs_grant_application(database& db)
    : _base_type(db)
{
}

const grant_application_object dbs_grant_application::create_grant_application(const external_id_type& funding_opportunity_number,
                                                                               const research_id_type& research_id,
                                                                               const std::string& application_hash,
                                                                               const account_name_type& creator)
{
    const auto& new_grant_application = db_impl().create<grant_application_object>([&](grant_application_object& ga) {
        auto now = db_impl().head_block_time();

        ga.funding_opportunity_number = funding_opportunity_number;
        ga.research_id = research_id;
        fc::from_string(ga.application_hash, application_hash);
        ga.creator = creator;
        ga.created_at = now;
        ga.status = grant_application_status::pending;
    });

    return new_grant_application;
}

const grant_application_object& dbs_grant_application::get_grant_application(const grant_application_id_type& id)
{
    try
    {
        return db_impl().get<grant_application_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_grant_application::grant_application_ref_type
dbs_grant_application::get_grant_application_if_exists(const grant_application_id_type& id) const
{
    grant_application_ref_type result;

    const auto& idx = db_impl()
      .get_index<grant_application_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_grant_application::grant_applications_refs_type
dbs_grant_application::get_grant_applications_by_funding_opportunity_number(const external_id_type& funding_opportunity_number)
{
    grant_applications_refs_type ret;

    auto it_pair = db_impl().get_index<grant_application_index>().indicies().get<by_funding_opportunity_number>().equal_range(funding_opportunity_number);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_grant_application::grant_applications_refs_type
dbs_grant_application::get_grant_applications_by_research_id(const research_id_type& research_id)
{
    grant_applications_refs_type ret;

    auto it_pair = db_impl().get_index<grant_application_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_grant_application::delete_grant_appication_by_id(const grant_application_id_type& grant_application_id)
{
    auto& grant_application = db_impl().get<grant_application_object, by_id>(grant_application_id);
    db_impl().remove(grant_application);
}

void dbs_grant_application::check_grant_application_existence(const grant_application_id_type& grant_application_id)
{
    auto grant_application = db_impl().find<grant_application_object, by_id>(grant_application_id);
    FC_ASSERT(grant_application != nullptr, "Grant application with id \"${1}\" must exist.",
              ("1", grant_application_id));
}

const bool dbs_grant_application::grant_application_exists(const grant_application_id_type& grant_application_id) const
{
    const auto& idx = db_impl()
            .get_index<grant_application_index>()
            .indices()
            .get<by_id>();

    auto itr = idx.find(grant_application_id);
    return itr != idx.end();
}

const grant_application_object& dbs_grant_application::update_grant_application_status(const grant_application_object& grant_application, const grant_application_status& new_status)
{
    db_impl().modify(grant_application, [&](grant_application_object& ga_o) { ga_o.status = new_status; });
    return grant_application;
}

const grant_application_review_object& dbs_grant_application::create_grant_application_review(const grant_application_id_type& grant_application_id,
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

const grant_application_review_object& dbs_grant_application::get_grant_application_review(const grant_application_review_id_type& id) const
{
    try
    {
        return db_impl().get<grant_application_review_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_grant_application::grant_application_review_ref_type
dbs_grant_application::get_grant_application_review_if_exists(const grant_application_review_id_type& id) const
{
    grant_application_review_ref_type result;

    const auto& idx = db_impl()
      .get_index<grant_application_review_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_grant_application::grant_application_review_refs_type
dbs_grant_application::get_grant_application_reviews(const grant_application_id_type& grant_application_id) const
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

dbs_grant_application::grant_application_review_refs_type
dbs_grant_application::get_author_reviews(const account_name_type& author) const
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

const bool dbs_grant_application::grant_application_review_exists(const grant_application_review_id_type& grant_application_review_id) const
{
    const auto& idx = db_impl()
            .get_index<grant_application_review_index>()
            .indices()
            .get<by_id>();

    auto itr = idx.find(grant_application_review_id);
    return itr != idx.end();
}

} // namespace chain
} // namespace deip