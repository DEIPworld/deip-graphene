#pragma once
#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <fc/shared_string.hpp>

namespace deip {
namespace chain {

typedef uint64_t asset_symbol_type;

class asset_object : public object<asset_object_type, asset_object>
{
    asset_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    asset_object(Constructor&& c, allocator<Allocator> a) : string_symbol(a), name(a), description(a)
    {
        c(*this);
    }

    asset_id_type id;

    asset_symbol_type symbol;
    fc::shared_string string_symbol;
    uint8_t precision;

    account_name_type issuer;
    fc::shared_string name;
    fc::shared_string description;

    share_type current_supply = 0;


    string to_string(const share_type& amount) const
    {
        share_type scaled_precision = pow(10, precision);
        string result = fc::to_string(amount.value / scaled_precision.value);

        auto decimals = abs( amount.value % scaled_precision.value );
        if( decimals )
        {
            if( amount < 0 && result == "0" )
                result = "-0";
            result += "." + fc::to_string(scaled_precision.value + decimals).erase(0,1);
        }
        result += " " + fc::to_string(symbol);

        return result;
    }
};

struct by_symbol;
struct by_string_symbol;
struct by_issuer;

typedef multi_index_container<asset_object,
            indexed_by<ordered_unique<tag<by_id>,
                    member<asset_object,
                            asset_id_type,
                           &asset_object::id>>,
            ordered_unique<tag<by_symbol>,
                    member<asset_object,
                            asset_symbol_type,
                           &asset_object::symbol>>,
            ordered_unique<tag<by_string_symbol>,
                    member<asset_object,
                            fc::shared_string,
                           &asset_object::string_symbol>>,
            ordered_non_unique<tag<by_issuer>,
                    member<asset_object,
                            account_name_type,
                           &asset_object::issuer>>>,
            allocator<asset_object>>
    asset_index;
    }
}

FC_REFLECT( deip::chain::asset_object, (id)(symbol)(string_symbol)(precision)(issuer)(name)(description)(current_supply))
CHAINBASE_SET_INDEX_TYPE( deip::chain::asset_object, deip::chain::asset_index )