
#pragma once
#include <deip/protocol/base.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {


struct update_proposal_operation: public base_operation
{
    external_id_type              external_id;
    flat_set<account_name_type>   posting_approvals_to_add;
    flat_set<account_name_type>   posting_approvals_to_remove;
    flat_set<account_name_type>   active_approvals_to_add;
    flat_set<account_name_type>   active_approvals_to_remove;
    flat_set<account_name_type>   owner_approvals_to_add;
    flat_set<account_name_type>   owner_approvals_to_remove;
    flat_set<public_key_type>     key_approvals_to_add;
    flat_set<public_key_type>     key_approvals_to_remove;
    extensions_type               extensions;

    void validate() const;

    void get_required_authorities( vector<authority>& o )const
    {
        authority auth;
        for( const auto& k : key_approvals_to_add )
            auth.key_auths[k] = 1;
        for( const auto& k : key_approvals_to_remove )
            auth.key_auths[k] = 1;
        auth.weight_threshold = auth.key_auths.size();

        if( auth.weight_threshold > 0 )
            o.emplace_back( std::move(auth) );
    }

    void get_required_posting_authorities( flat_set<account_name_type>& a )const
    {
        for( const auto& i : posting_approvals_to_add )    a.insert(i);
        for( const auto& i : posting_approvals_to_remove ) a.insert(i);
    }

    void get_required_active_authorities( flat_set<account_name_type>& a )const
    {
        for( const auto& i : active_approvals_to_add )    a.insert(i);
        for( const auto& i : active_approvals_to_remove ) a.insert(i);
    }

    void get_required_owner_authorities( flat_set<account_name_type>& a )const
    {
        for( const auto& i : owner_approvals_to_add )    a.insert(i);
        for( const auto& i : owner_approvals_to_remove ) a.insert(i);
    }
};

}
}

FC_REFLECT( deip::protocol::update_proposal_operation,
  (external_id)
  (active_approvals_to_add)
  (active_approvals_to_remove)
  (owner_approvals_to_add)
  (owner_approvals_to_remove)
  (key_approvals_to_add)
  (key_approvals_to_remove)
  (extensions) 
)
