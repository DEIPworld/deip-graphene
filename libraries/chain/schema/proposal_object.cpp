#include <deip/chain/database/database.hpp>
#include <deip/protocol/authority.hpp>
#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/schema/account_object.hpp>

namespace deip {
namespace chain {

bool proposal_object::is_authorized_to_execute(chainbase::database& db) const
{
    auto get_active
        = [&](const string& name) { return authority(db.get<account_authority_object, by_account>(name).active); };
    auto get_owner
        = [&](const string& name) { return authority(db.get<account_authority_object, by_account>(name).owner); };
    auto get_posting
        = [&](const string& name) { return authority(db.get<account_authority_object, by_account>(name).posting); };

   try {
      verify_authority( proposed_transaction.operations, 
                        available_key_approvals,
                        get_active,
                        get_owner,
                        get_posting,
                        DEIP_MAX_SIG_CHECK_DEPTH,
                        available_active_approvals,
                        available_owner_approvals);
   } 
   catch ( const fc::exception& e )
   {
      return false;
   }
   return true;
}



}
} // deip::chain
