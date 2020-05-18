#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

using protocol::external_id_type;

enum class award_withdrawal_request_status : uint16_t
{
    pending = 1,
    certified = 2,
    approved = 3,
    paid = 4,
    rejected = 5
};

class award_withdrawal_request_object : public object<award_withdrawal_request_object_type, award_withdrawal_request_object>
{
    award_withdrawal_request_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    award_withdrawal_request_object(Constructor&& c, allocator<Allocator> a)
        : description(a)
        , attachment(a)
    {
        c(*this);
    }

    award_withdrawal_request_id_type id;

    external_id_type payment_number;
    external_id_type award_number;
    external_id_type subaward_number;

    account_name_type requester;
    asset amount = asset(0, DEIP_SYMBOL);
    uint16_t status = static_cast<uint16_t>(award_withdrawal_request_status::pending);
    fc::time_point_sec time;

    fc::shared_string description;
    fc::shared_string attachment;
};

struct by_research;
struct by_award_number_and_status;
struct by_award_and_payment_number;
struct by_award_number;
struct by_award_and_subaward_number;

typedef multi_index_container<award_withdrawal_request_object,
          indexed_by<
            ordered_unique<
              tag<by_id>,
                member<
                  award_withdrawal_request_object,
                  award_withdrawal_request_id_type,
                  &award_withdrawal_request_object::id
                >
            >,
            ordered_unique<
              tag<by_award_and_payment_number>,
                composite_key<award_withdrawal_request_object,
                  member<
                    award_withdrawal_request_object,
                    external_id_type,
                    &award_withdrawal_request_object::award_number
                  >,
                  member<
                    award_withdrawal_request_object,
                    external_id_type,
                    &award_withdrawal_request_object::payment_number
                  >
                >
            >,
            ordered_non_unique<
              tag<by_award_and_subaward_number>,
                composite_key<award_withdrawal_request_object,
                  member<
                    award_withdrawal_request_object,
                    external_id_type,
                    &award_withdrawal_request_object::award_number
                  >,
                  member<
                    award_withdrawal_request_object,
                    external_id_type,
                    &award_withdrawal_request_object::subaward_number
                  >
                >
            >,
            ordered_non_unique<
              tag<by_award_number_and_status>,
                composite_key<award_withdrawal_request_object,
                  member<
                    award_withdrawal_request_object,
                    external_id_type,
                    &award_withdrawal_request_object::award_number
                  >,
                  member<
                    award_withdrawal_request_object,
                    uint16_t,
                    &award_withdrawal_request_object::status
                  >
                >,
                composite_key_compare<
                  std::less<external_id_type>,
                  std::less<uint16_t>
                >
            >,
            ordered_non_unique<
              tag<by_award_number>,
                member<
                  award_withdrawal_request_object,
                  external_id_type,
                  &award_withdrawal_request_object::award_number
                >
            >
            >,
        allocator<award_withdrawal_request_object>>
        award_withdrawal_request_index;

}
}

FC_REFLECT_ENUM(deip::chain::award_withdrawal_request_status,
  (pending)
  (certified)
  (approved)
  (paid)
  (rejected)
)

FC_REFLECT( deip::chain::award_withdrawal_request_object,
  (id)
  (payment_number)
  (award_number)
  (subaward_number)
  (requester)
  (amount)
  (description)
  (status)
  (time)
  (attachment)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::award_withdrawal_request_object, deip::chain::award_withdrawal_request_index )
