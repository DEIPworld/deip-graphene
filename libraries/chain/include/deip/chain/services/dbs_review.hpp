#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/review_object.hpp>

namespace deip {
namespace chain {

///** DB service for operations with review_object
// *  --------------------------------------------
// */
class dbs_review : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_review() = delete;

protected:
    explicit dbs_review(database &db);

public:
    using review_refs_type = std::vector<std::reference_wrapper<const review_object>>;
    using review_optional_ref_type = fc::optional<std::reference_wrapper<const review_object>>;

    const review_object& get_review(const review_id_type& id) const;
    const review_object& get_review(const external_id_type& external_id) const;

    const review_optional_ref_type get_review_if_exists(const review_id_type& id) const;
    const review_optional_ref_type get_review_if_exists(const external_id_type& external_id) const;

    review_refs_type get_reviews_by_research_content(const research_content_id_type& research_content_id) const;
    review_refs_type get_reviews_by_research_content(const external_id_type& research_content_external_id) const;

    review_refs_type get_reviews_by_research(const external_id_type& research_external_id) const;

    review_refs_type get_author_reviews(const account_name_type& author) const;

    const review_object& create_review(const external_id_type& external_id,
                                       const external_id_type& research_external_id,
                                       const external_id_type& research_content_external_id,
                                       const research_content_id_type& research_content_id,
                                       const string& content,
                                       const bool& is_positive,
                                       const account_name_type& author,
                                       const std::set<discipline_id_type>& disciplines,
                                       const std::map<discipline_id_type, share_type>& used_expertise,
                                       const int32_t& assessment_model_v,
                                       const fc::optional<std::map<assessment_criteria, assessment_criteria_value>>& scores);

    const std::map<discipline_id_type, share_type> get_eci_weight(const review_id_type& review_id) const;
    
};
} // namespace chain
} // namespace deip
