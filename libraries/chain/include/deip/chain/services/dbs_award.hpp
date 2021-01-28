#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/award_object.hpp>
#include <deip/chain/schema/award_recipient_object.hpp>
#include <deip/chain/schema/award_withdrawal_request_object.hpp>
#include <deip/protocol/percent.hpp>
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
    using award_optional_ref_type = fc::optional<std::reference_wrapper<const award_object>>;
    using award_recipient_refs_type = std::vector<std::reference_wrapper<const award_recipient_object>>;
    using award_recipient_optional_ref_type = fc::optional<std::reference_wrapper<const award_recipient_object>>;
    using award_withdrawal_request_refs_type = std::vector<std::reference_wrapper<const award_withdrawal_request_object>>;
    using award_withdrawal_request_ref_optional_type = fc::optional<std::reference_wrapper<const award_withdrawal_request_object>>;

    const award_object& create_award(const external_id_type& funding_opportunity_number,
                                     const external_id_type& award_number,
                                     const account_name_type& awardee,
                                     const asset& amount,
                                     const research_group_id_type& university_id,
                                     const external_id_type& university_external_id,
                                     const percent& university_overhead,
                                     const account_name_type& creator,
                                     const award_status& status);

    const award_object& get_award(const award_id_type& id) const;

    const award_object& get_award(const external_id_type& number) const;

    const award_optional_ref_type get_award_if_exists(const award_id_type& id) const;

    const award_optional_ref_type get_award_if_exists(const external_id_type& number) const;

    award_refs_type get_awards_by_funding_opportunity(const external_id_type& funding_opportunity_number) const;

    const award_object& update_award_status(const award_object& award, const award_status& status);

    const bool award_exists(const award_id_type& award_id) const;

    const bool award_exists(const external_id_type& award_number) const;

    // Award recipients

    const award_recipient_object& create_award_recipient(
      const external_id_type& award_number,
      const external_id_type& subaward_number,
      const external_id_type& funding_opportunity_number,
      const account_name_type& awardee,
      const account_name_type& source,
      const asset& total_amount,
      const research_id_type& research_id,
      const external_id_type& research_external_id,
      const award_recipient_status& status);

    const award_recipient_object& get_award_recipient(const award_recipient_id_type& id) const;

    const award_recipient_object& get_award_recipient(const external_id_type& award_number, const external_id_type& subaward_number) const;

    const award_recipient_optional_ref_type get_award_recipient_if_exists(const award_recipient_id_type& id) const;

    award_recipient_refs_type get_award_recipients_by_account(const account_name_type& awardee) const;

    award_recipient_refs_type get_award_recipients_by_award(const external_id_type& award_number) const;

    award_recipient_refs_type get_award_recipients_by_funding_opportunity(const external_id_type& funding_opportunity_number) const;

    const bool award_recipient_exists(const award_recipient_id_type& award_recipient_id) const;

    const bool award_recipient_exists(const external_id_type& award_number, const external_id_type& subaward_number) const;

    const award_recipient_object& adjust_expenses(const award_recipient_id_type& award_recipient_id, const asset& delta);

    const award_recipient_object& update_award_recipient_status(const award_recipient_object& award_recipient, const award_recipient_status& status);

    // Award withdrawal requests

    const award_withdrawal_request_object& create_award_withdrawal_request(const external_id_type& payment_number,
                                                                           const external_id_type& award_number,
                                                                           const external_id_type& subaward_number,
                                                                           const account_name_type& requester,
                                                                           const asset& amount,
                                                                           const std::string& description,
                                                                           const fc::time_point_sec& time,
                                                                           const std::string& attachment);
    
    const award_withdrawal_request_object& get_award_withdrawal_request(const external_id_type & award_number, const external_id_type& payment_number) const;

    const award_withdrawal_request_ref_optional_type get_award_withdrawal_request_if_exists(const external_id_type& award_number, const external_id_type& payment_number) const;

    const bool award_withdrawal_request_exists(const external_id_type& award_number, const external_id_type& payment_number) const;

    const award_withdrawal_request_object& update_award_withdrawal_request(const award_withdrawal_request_object& award_withdrawal_request, const award_withdrawal_request_status& status);

    award_withdrawal_request_refs_type get_award_withdrawal_requests_by_award(const external_id_type& award_number) const;
    
    award_withdrawal_request_refs_type get_award_withdrawal_requests_by_award_and_subaward(const external_id_type& award_number, const external_id_type& subaward_number) const;

    award_withdrawal_request_refs_type get_award_withdrawal_requests_by_award_and_status(const external_id_type& award_number, const award_withdrawal_request_status& status) const;
};
}
}
