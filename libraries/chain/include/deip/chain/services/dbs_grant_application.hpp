#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/grant_application_object.hpp>
#include <deip/chain/schema/research_object.hpp>

#include <vector>


namespace deip{
namespace chain{

class dbs_grant_application: public dbs_base
{

    friend class dbservice_dbs_factory;

    dbs_grant_application() = delete;

protected:
    explicit dbs_grant_application(database& db);

public:

    using grant_applications_refs_type = std::vector<std::reference_wrapper<const grant_application_object>>;

    const grant_application_object create_grant_application(const grant_id_type& grant_id,
                                                            const research_id_type& research_id,
                                                            const std::string& application_hash,
                                                            const account_name_type& creator);

    const grant_application_object& get_grant_application(const grant_application_id_type& id);

    grant_applications_refs_type get_grant_applications_by_grant(const grant_id_type& grant_id);

    grant_applications_refs_type get_grant_applications_by_research_id(const research_id_type& research_id);

    void delete_grant_appication_by_id(const grant_application_id_type& grant_application_id);

    void check_grant_application_existence(const grant_application_id_type& grant_application_id);

    const grant_application_object& update_grant_application_status(const grant_application_object& grant_application, const grant_application_status& new_status);
};
}
}