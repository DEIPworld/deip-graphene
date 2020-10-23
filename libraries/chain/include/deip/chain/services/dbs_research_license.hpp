#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/research_license_object.hpp>
#include <deip/chain/schema/research_object.hpp>

#include <vector>

namespace deip {
namespace chain {

class dbs_research_license : public dbs_base {

    friend class dbservice_dbs_factory;

    dbs_research_license() = delete;

protected:

    explicit dbs_research_license(database &db);

public:
    using research_license_refs_type = std::vector<std::reference_wrapper<const research_license_object>>;
    using research_license_optional_ref_type = fc::optional<std::reference_wrapper<const research_license_object>>;

    const research_license_object& create_research_license(const research_object& research,
                                                           const external_id_type& external_id,
                                                           const account_name_type& licensee,
                                                           const string& terms,
                                                           const optional<time_point_sec>& expiration_time,
                                                           const optional<asset>& fee);

    const research_license_refs_type get_research_licenses_by_research_group(const account_name_type& research_group) const;

    const research_license_object& get_research_license(const external_id_type& external_id) const;

    const research_license_optional_ref_type get_research_license_if_exists(const external_id_type& external_id) const;

    const research_license_refs_type get_research_licenses_by_research(const external_id_type& research_external_id) const;

    const research_license_refs_type get_research_licenses_by_licensee(const account_name_type& licensee) const;

    const bool research_license_exists(const external_id_type& external_id) const;

    const research_license_refs_type get_research_licenses_by_licensee_and_research(const account_name_type& licensee, const external_id_type& research_external_id) const;

    const research_license_refs_type get_research_licenses_by_licensee_and_research_group(const account_name_type& licensee, const account_name_type& research_group) const;
};
}
}