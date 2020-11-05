#pragma once
#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

using fc::shared_string;
using deip::protocol::asset;
using deip::protocol::external_id_type;

class account_balance_object : public object<account_balance_object_type, account_balance_object>
{
    account_balance_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    account_balance_object(Constructor&& c, allocator<Allocator> a)
        : string_symbol(a)
    {
        c(*this);
    }

    account_balance_id_type id;
    asset_id_type asset_id;

    asset_symbol_type symbol;
    shared_string string_symbol;

    account_name_type owner;
    share_type amount = 0;
    share_type frozen_amount = 0;

    optional<external_id_type> tokenized_research;

    const asset to_asset() const 
    {
        return asset(amount, symbol);
    }
};

struct by_owner;
struct by_symbol;
struct by_string_symbol;
struct by_owner_and_asset_symbol;
struct by_owner_and_asset_string_symbol;

typedef multi_index_container<account_balance_object,
            indexed_by<
            
            ordered_unique<tag<by_id>,
                    member<account_balance_object,
                            account_balance_id_type,
                           &account_balance_object::id>>,

            ordered_non_unique<tag<by_owner>,
                    member<account_balance_object,
                           account_name_type,
                           &account_balance_object::owner>>,

            ordered_non_unique<tag<by_symbol>,
                    member<account_balance_object,
                           asset_symbol_type,
                           &account_balance_object::symbol>>,

            ordered_non_unique<tag<by_string_symbol>,
                    member<account_balance_object,
                           shared_string,
                           &account_balance_object::string_symbol>>,

            ordered_unique<tag<by_owner_and_asset_symbol>,
                    composite_key<account_balance_object,
                            member<account_balance_object,
                                   account_name_type,
                                   &account_balance_object::owner>,
                            member<account_balance_object,
                                    asset_symbol_type,
                                    &account_balance_object::symbol>>>,

            ordered_unique<tag<by_owner_and_asset_string_symbol>,
                    composite_key<account_balance_object,
                            member<account_balance_object,
                                    account_name_type,
                                    &account_balance_object::owner>,
                            member<account_balance_object,
                                    shared_string,
                                    &account_balance_object::string_symbol>>,
                    composite_key_compare<std::less<account_name_type>, fc::strcmp_less>>
            >,
        allocator<account_balance_object>>
        
        account_balance_index;
    }
}

FC_REFLECT(deip::chain::account_balance_object, (id)(asset_id)(symbol)(string_symbol)(owner)(amount)(frozen_amount)(tokenized_research))
CHAINBASE_SET_INDEX_TYPE( deip::chain::account_balance_object, deip::chain::account_balance_index )