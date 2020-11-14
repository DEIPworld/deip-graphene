#pragma once
#include <deip/chain/schema/deip_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace account_by_key {

using namespace std;
using namespace deip::chain;

#ifndef ACCOUNT_BY_KEY_SPACE_ID
#define ACCOUNT_BY_KEY_SPACE_ID 11
#endif

enum account_by_key_object_types
{
    key_lookup_object_type = (ACCOUNT_BY_KEY_SPACE_ID << 8)
};

class key_lookup_object : public object<key_lookup_object_type, key_lookup_object>
{
public:
    template <typename Constructor, typename Allocator> key_lookup_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    id_type id;

    public_key_type key;
    account_name_type account;
    time_point_sec deactivation_time;
};

typedef key_lookup_object::id_type key_lookup_id_type;

using namespace boost::multi_index;

struct by_key;
struct by_key_deactivation_time;

typedef multi_index_container<key_lookup_object,
    indexed_by<
        ordered_unique<tag<by_id>, 
            member<key_lookup_object, 
                    key_lookup_id_type, 
                    &key_lookup_object::id>
        >,

        ordered_non_unique<tag<by_key>,
            composite_key<key_lookup_object, 
                member<key_lookup_object, 
                        public_key_type,
                        &key_lookup_object::key>,
                member<key_lookup_object,
                        account_name_type,
                        &key_lookup_object::account>
            >
        >,              
        
        ordered_unique<tag<by_key_deactivation_time>,
            composite_key<key_lookup_object, 
                member<key_lookup_object, 
                        public_key_type,
                        &key_lookup_object::key>,
                member<key_lookup_object,
                        account_name_type,
                        &key_lookup_object::account>,
                member<key_lookup_object,
                        time_point_sec,
                        &key_lookup_object::deactivation_time>
            >
        >
        
    >,
    allocator<key_lookup_object>>
    key_lookup_index;
}
} // deip::account_by_key

FC_REFLECT(deip::account_by_key::key_lookup_object, (id)(key)(account)(deactivation_time))
CHAINBASE_SET_INDEX_TYPE(deip::account_by_key::key_lookup_object, deip::account_by_key::key_lookup_index)
