#pragma once

#include <boost/multi_index/composite_key.hpp>

using namespace deip::chain;
using namespace std;

namespace deip {
namespace fo_history {

using chainbase::allocator;

class withdrawal_request_history_object : public object<withdrawal_request_history_object_type, withdrawal_request_history_object>
{
public:
    template <typename Constructor, typename Allocator>
    withdrawal_request_history_object(Constructor&& c, allocator<Allocator> a) : payment_number(a), award_number(a), subaward_number(a), description(a), attachment(a)
    {
        c(*this);
    }

    withdrawal_request_history_id_type id;

    fc::shared_string payment_number;
    fc::shared_string award_number;
    fc::shared_string subaward_number;

    account_name_type requester;
    account_name_type certifier;
    account_name_type approver;
    account_name_type rejector;
    account_name_type payer;

    asset amount;

    fc::shared_string description;
    fc::shared_string attachment;

    uint16_t status = static_cast<uint16_t>(award_withdrawal_request_status::pending);
    uint16_t version;

    transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;
};

struct by_award_number;
struct by_award_and_payment_number;
struct by_award_and_subaward_number;
struct by_award_payment_and_version;

typedef chainbase::shared_multi_index_container<withdrawal_request_history_object,
    indexed_by<
        ordered_unique<
            tag<by_id>,
                member<
                    withdrawal_request_history_object,
                    withdrawal_request_history_id_type,
                    &withdrawal_request_history_object::id
                >
        >,
        ordered_non_unique<
            tag<by_award_number>,
                member<
                    withdrawal_request_history_object,
                    fc::shared_string,
                    &withdrawal_request_history_object::award_number
                >,
                fc::strcmp_less
        >,
        ordered_non_unique<
              tag<by_award_and_payment_number>,
                  composite_key<withdrawal_request_history_object,
                      member<
                          withdrawal_request_history_object,
                          fc::shared_string,
                          &withdrawal_request_history_object::award_number
                      >,
                      member<
                          withdrawal_request_history_object,
                          fc::shared_string,
                          &withdrawal_request_history_object::payment_number
                      >
                  >,
                  composite_key_compare<
                      fc::strcmp_less,
                      fc::strcmp_less
                  >
        >,
        ordered_non_unique<
              tag<by_award_and_subaward_number>,
                  composite_key<withdrawal_request_history_object,
                      member<
                          withdrawal_request_history_object,
                          fc::shared_string,
                          &withdrawal_request_history_object::award_number
                      >,
                      member<
                          withdrawal_request_history_object,
                          fc::shared_string,
                          &withdrawal_request_history_object::subaward_number
                      >
                  >,
                  composite_key_compare<
                      fc::strcmp_less,
                      fc::strcmp_less
                  >
        >,
        ordered_unique<
            tag<by_award_payment_and_version>,
                composite_key<withdrawal_request_history_object,
                    member<
                        withdrawal_request_history_object,
                        fc::shared_string,
                        &withdrawal_request_history_object::award_number>,
                    member<
                        withdrawal_request_history_object,
                        fc::shared_string,
                        &withdrawal_request_history_object::payment_number>,
                    member<
                        withdrawal_request_history_object,
                        uint16_t,
                        &withdrawal_request_history_object::version>
                >,
                composite_key_compare<
                        fc::strcmp_less,
                        fc::strcmp_less,
                        std::less<uint32_t>
                >
            >
        >
    >
    withdrawal_request_history_index;

} // namespace fo_history
} // namespace deip

FC_REFLECT(deip::fo_history::withdrawal_request_history_object,
    (id)
    (payment_number)
    (award_number)
    (subaward_number)
    (requester)
    (certifier)
    (approver)
    (rejector)
    (payer)
    (amount)
    (description)
    (attachment)
    (status)
    (trx_id)
    (block)
    (trx_in_block)
    (op_in_trx)
)

CHAINBASE_SET_INDEX_TYPE(deip::fo_history::withdrawal_request_history_object, deip::fo_history::withdrawal_request_history_index)