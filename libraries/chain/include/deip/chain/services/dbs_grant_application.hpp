#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/grant_application_object.hpp>
#include <deip/chain/schema/grant_application_review_object.hpp>
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
    using grant_application_ref_type = fc::optional<std::reference_wrapper<const grant_application_object>>;
    using grant_application_review_refs_type = std::vector<std::reference_wrapper<const grant_application_review_object>>;
    using grant_application_review_ref_type = fc::optional<std::reference_wrapper<const grant_application_review_object>>;

    const grant_application_object create_grant_application(const external_id_type& funding_opportunity_number,
                                                            const research_id_type& research_id,
                                                            const std::string& application_hash,
                                                            const account_name_type& creator);

    const grant_application_object& get_grant_application(const grant_application_id_type& id);

    const grant_application_ref_type get_grant_application_if_exists(const grant_application_id_type& id) const;

    grant_applications_refs_type get_grant_applications_by_funding_opportunity_number(const external_id_type& funding_opportunity_number);

    grant_applications_refs_type get_grant_applications_by_research_id(const research_id_type& research_id);

    void delete_grant_appication_by_id(const grant_application_id_type& grant_application_id);

    void check_grant_application_existence(const grant_application_id_type& grant_application_id);

    const bool grant_application_exists(const grant_application_id_type& grant_application_id) const;

    const grant_application_object& update_grant_application_status(const grant_application_object& grant_application, const grant_application_status& new_status);

    // Grant application review

    const grant_application_review_object& create_grant_application_review(const grant_application_id_type& grant_application_id,
                                                                           const string& content,
                                                                           bool is_positive,
                                                                           const account_name_type& author,
                                                                           const std::set<discipline_id_type>& disciplines);

    const grant_application_review_object& get_grant_application_review(const grant_application_review_id_type& id) const;

    const grant_application_review_ref_type get_grant_application_review_if_exists(const grant_application_review_id_type& id) const;

    grant_application_review_refs_type get_grant_application_reviews(const grant_application_id_type& grant_application_id) const;

    grant_application_review_refs_type get_author_reviews(const account_name_type& author) const;

    const bool grant_application_review_exists(const grant_application_review_id_type& grant_application_review_id) const;
};
}
}
