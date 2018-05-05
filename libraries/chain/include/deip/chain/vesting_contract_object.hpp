#pragma once

#include <deip/protocol/types.hpp>
#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip{
namespace chain{

class vesting_contract_object : public object<vesting_contract_object_type, vesting_contract_object>
{

    vesting_contract_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    vesting_contract_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    vesting_contract_id_type id;
    account_name_type sender;
    account_name_type receiver;
    share_type amount;
    share_type withdrawn;
    uint32_t withdrawal_periods;
    time_point_sec start_date;
    time_point_sec expiration_date;
    time_point_sec contract_duration;
};

struct by_sender_and_receiver;
struct by_receiver;

typedef multi_index_container<vesting_contract_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<vesting_contract_object,
                                    vesting_contract_id_type,
                                    &vesting_contract_object::id>>,
                        ordered_unique<tag<by_sender_and_receiver>,
                             composite_key<vesting_contract_object,
                             member<vesting_contract_object,
                                    account_name_type,
                                    &vesting_contract_object::sender>,
                             member<vesting_contract_object,
                                     account_name_type,
                                    &vesting_contract_object::receiver>>>,
                        ordered_non_unique<tag<by_receiver>,
                                        member<vesting_contract_object,
                                                account_name_type,
                                                &vesting_contract_object::receiver>>>,

                        allocator<vesting_contract_object>>
                        vesting_contract_index;
}
}

FC_REFLECT(deip::chain::vesting_contract_object,
                        (id)(sender)(receiver)(amount)(withdrawal_periods)(start_date)(expiration_date)(contract_duration)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::vesting_contract_object, deip::chain::vesting_contract_index)