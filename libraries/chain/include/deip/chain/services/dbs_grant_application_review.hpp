#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/grant_application_object.hpp>
#include <deip/chain/schema/grant_application_review_object.hpp>

namespace deip {
namespace chain {

///** DB service for operations with grant_application_review_object
// *  --------------------------------------------
// */
class dbs_grant_application_review : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_grant_application_review() = delete;

protected:
    explicit dbs_grant_application_review(database& db);

public:
    using grant_application_review_refs_type = std::vector<std::reference_wrapper<const grant_application_review_object>>;

    const grant_application_review_object& create(const grant_application_id_type& grant_application_id,
                                                  const string& content,
                                                  bool is_positive,
                                                  const account_name_type& author,
                                                  const std::set<discipline_id_type>& disciplines);
                                                  
    const grant_application_review_object& get(const grant_application_review_id_type& id) const;

    grant_application_review_refs_type get_grant_application_reviews(const grant_application_id_type& grant_application_id) const;

    grant_application_review_refs_type get_author_reviews(const account_name_type& author) const;

};
} // namespace chain
} // namespace deip
