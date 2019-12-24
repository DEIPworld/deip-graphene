#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>

namespace deip {
namespace chain {

dbs_grant_application::dbs_grant_application(database& db)
    : _base_type(db)
{
}

const grant_application_object dbs_grant_application::create_grant_application(const grant_id_type& grant_id,
                                                                               const research_id_type& research_id,
                                                                               const std::string& application_hash,
                                                                               const account_name_type& creator)
{
    const auto& new_grant_application = db_impl().create<grant_application_object>([&](grant_application_object& ga) {
        auto now = db_impl().head_block_time();

        ga.grant_id = grant_id;
        ga.research_id = research_id;
        fc::from_string(ga.application_hash, application_hash);
        ga.creator = creator;
        ga.created_at = now;
        ga.status = grant_application_status::application_pending;
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

dbs_grant_application::grant_applications_refs_type dbs_grant_application::get_grant_applications_by_grant(const grant_id_type& grant_id)
{
    grant_applications_refs_type ret;

    auto it_pair = db_impl().get_index<grant_application_index>().indicies().get<by_grant_id>().equal_range(grant_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_grant_application::grant_applications_refs_type dbs_grant_application::get_grant_applications_by_research_id(const research_id_type& research_id)
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

const grant_application_object&dbs_grant_application::update_grant_application_status(const grant_application_object& grant_application, const grant_application_status& new_status)
{
    db_impl().modify(grant_application, [&](grant_application_object& ga_o) { ga_o.status = new_status; });
    return grant_application;
}

} // namespace chain
} // namespace deip