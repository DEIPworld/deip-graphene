#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/award_object.hpp>
#include <deip/chain/schema/award_recipient_object.hpp>

#include <vector>

namespace deip{
namespace chain{

class dbs_award : public dbs_base{

    friend class dbservice_dbs_factory;

    dbs_award() = delete;

protected:

    explicit dbs_award(database &db);

public:

    using award_refs_type = std::vector<std::reference_wrapper<const award_object>>;
    using award_optional_type = fc::optional<std::reference_wrapper<const award_object>>;
    using award_recipient_refs_type = std::vector<std::reference_wrapper<const award_recipient_object>>;
    using award_recipient_optional_type = fc::optional<std::reference_wrapper<const award_recipient_object>>;

    const award_object& create_award(const funding_opportunity_id_type& funding_opportunity_id,
                                     const account_name_type& creator,
                                     const asset& amount);

    const award_object& get_award(const award_id_type& id) const;

    const award_optional_type get_award_if_exists(const award_id_type& id) const;

    award_refs_type get_awards_by_funding_opportunity(const funding_opportunity_id_type& funding_opportunity_id) const;

    // Award recipients


    const award_recipient_object& create_award_recipient(const award_id_type& award_id,
                                                         const funding_opportunity_id_type& funding_opportunity_id,
                                                         const research_id_type& research_id,
                                                         const research_group_id_type& research_group_id,
                                                         const account_name_type& awardee,
                                                         const asset& total_amount,
                                                         const research_group_id_type& university_id,
                                                         const share_type& university_overhead);


    const award_recipient_object& get_award_recipient(const award_recipient_id_type& id);

    const award_recipient_optional_type get_award_recipient_if_exists(const award_recipient_id_type& id);

    award_recipient_refs_type get_award_recipients_by_account(const account_name_type& awardee);

    award_recipient_refs_type get_award_recipients_by_award(const award_id_type& award_id);

    award_recipient_refs_type get_award_recipients_by_funding_opportunity(const funding_opportunity_id_type& funding_opportunity_id);


};
}
}
