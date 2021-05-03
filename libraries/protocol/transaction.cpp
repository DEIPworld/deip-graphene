
#include <deip/protocol/transaction.hpp>
#include <deip/protocol/exceptions.hpp>

#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <fc/smart_ref_impl.hpp>

#include <algorithm>

namespace deip {
namespace protocol {

digest_type signed_transaction::merkle_digest() const
{
    digest_type::encoder enc;
    fc::raw::pack(enc, *this);
    return enc.result();
}

digest_type transaction::digest() const
{
    digest_type::encoder enc;
    fc::raw::pack(enc, *this);
    return enc.result();
}

digest_type transaction::sig_digest(const chain_id_type& chain_id) const
{
    digest_type::encoder enc;
    fc::raw::pack(enc, chain_id);
    fc::raw::pack(enc, *this);
    return enc.result();
}

void transaction::validate() const
{
    FC_ASSERT(operations.size() > 0, "A transaction must have at least one operation", ("trx", *this));
    for (const auto& op : operations)
    {
        operation_validate(op);
        entity_validate(op, ref_block_num, ref_block_prefix);
    }
}

deip::protocol::transaction_id_type deip::protocol::transaction::id() const
{
    auto h = digest();
    transaction_id_type result;
    memcpy(result._hash, h._hash, std::min(sizeof(result), sizeof(h)));
    return result;
}

const signature_type& deip::protocol::signed_transaction::sign(const private_key_type& key,
                                                                 const chain_id_type& chain_id)
{
    digest_type h = sig_digest(chain_id);
    signatures.push_back(key.sign_compact(h));
    return signatures.back();
}

signature_type deip::protocol::signed_transaction::sign(const private_key_type& key,
                                                          const chain_id_type& chain_id) const
{
    digest_type::encoder enc;
    fc::raw::pack(enc, chain_id);
    fc::raw::pack(enc, *this);
    return key.sign_compact(enc.result());
}

void transaction::set_expiration(fc::time_point_sec expiration_time)
{
    expiration = expiration_time;
}

void transaction::set_reference_block(const block_id_type& reference_block)
{
    ref_block_num = fc::endian_reverse_u32(reference_block._hash[0]);
    ref_block_prefix = reference_block._hash[1];
}

void transaction::get_required_authorities(flat_set<account_name_type>& active,
                                           flat_set<account_name_type>& owner,
                                           vector<authority>& other) const
{
    for (const auto& op : operations)
        operation_get_required_authorities(op, active, owner, other);
}

void verify_authority(const vector<operation>& tx_ops,
                      const flat_set<public_key_type>& sigs,
                      const authority_getter& get_active,
                      const authority_getter& get_owner,
                      const override_authority_getter& get_active_overrides,
                      const flat_set<account_name_type>& available_active_approvals,
                      const flat_set<account_name_type>& available_owner_approvals)
{
    try
    {
        flat_set<account_name_type> required_active;
        flat_set<account_name_type> required_owner;
        vector<authority> other;
        vector<std::pair<account_name_type, authority>> active_overrides;

        flat_map<account_name_type, authority_pack> new_accounts;
        vector<std::pair<account_name_type, authority>> required_new_active;
        vector<std::pair<account_name_type, authority>> required_new_owner;

        extract_new_accounts(tx_ops, new_accounts);

        for (const auto& op : tx_ops)
        {
            flat_set<account_name_type> op_required_active;
            flat_set<account_name_type> op_required_owner;

            operation_get_required_authorities(op, op_required_active, op_required_owner, other);

            for (auto itr = op_required_active.begin(); itr != op_required_active.end();)
            {
                if (new_accounts.find(*itr) != new_accounts.end())
                {
                    auto auths_pack = new_accounts.at(*itr);
                    uint16_t op_tag = (uint16_t)op.which();
                    if (auths_pack.active_overrides.find(op_tag) != auths_pack.active_overrides.end())
                    {
                        auto active_override = auths_pack.active_overrides.at(op_tag);
                        required_new_active.push_back(std::make_pair(*itr, active_override));
                    } 
                    else 
                    {
                        required_new_active.push_back(std::make_pair(*itr, auths_pack.active));
                    } 

                    itr = op_required_active.erase(itr);
                }
                else
                {
                    ++itr;
                }
            }

            for (auto itr = op_required_active.begin(); itr != op_required_active.end();)
            {
                uint16_t op_tag = (uint16_t)op.which();
                const auto& auth_opt = get_active_overrides(*itr, op_tag);
                if (auth_opt.valid())
                {
                    active_overrides.push_back(std::make_pair(*itr, *auth_opt)); // TODO: remove duplicates
                    itr = op_required_active.erase(itr);
                }
                else
                {
                    ++itr;
                }
            }

            for (auto itr = op_required_owner.begin(); itr != op_required_owner.end();)
            {
                if (new_accounts.find(*itr) != new_accounts.end())
                {
                    auto auths_pack = new_accounts.at(*itr);
                    required_new_owner.push_back(std::make_pair(*itr, auths_pack.owner));
                    itr = op_required_owner.erase(itr);
                }
                else
                {
                    ++itr;
                }
            }

            required_active.insert(op_required_active.begin(), op_required_active.end());
            required_owner.insert(op_required_owner.begin(), op_required_owner.end());
        }

        flat_set<public_key_type> avail;
        sign_state s(sigs, get_active, get_owner, avail);

        for (auto& id : available_active_approvals)
            s.approved_by.insert(id);
        for (auto& id : available_owner_approvals)
            s.approved_by.insert(id);

        for (const auto& auth : other)
        {
            DEIP_ASSERT(
              // TODO: implement inclusive authority checker to check available_key_approvals
              s.check_authority(auth), 
              tx_missing_other_auth, 
              "Missing Authority", 
              ("auth", auth)
              ("sigs", sigs)
            );
        }

        for (const auto& pair : active_overrides)
        {
            DEIP_ASSERT(
              // TODO: implement inclusive authority checker to check available_key_approvals
              available_owner_approvals.find(pair.first) != available_owner_approvals.end() ||
              s.check_authority(pair.second) ||
              s.check_authority(get_owner(pair.first)), 
              tx_missing_other_auth, 
              "Missing Overridden Authority", 
              ("id", pair.first)
              ("auth", pair.second)
              ("sigs", sigs)
            );
        }

        for (const auto& pair : required_new_active)
        {
            DEIP_ASSERT(
              s.check_authority(pair.second) ||
              s.check_authority(new_accounts.at(pair.first).owner), 
              tx_missing_other_auth, 
              "Missing New Account Active Authority", 
              ("id", pair.first)
              ("auth", pair.second)
              ("sigs", sigs)
            );
        }

        for (const auto& pair : required_new_owner)
        {
            DEIP_ASSERT(
              s.check_authority(pair.second), 
              tx_missing_other_auth, 
              "Missing New Account Owner Authority", 
              ("id", pair.first)
              ("auth", pair.second)
              ("sigs", sigs)
            );
        }

        for (auto id : required_active)
        {
            DEIP_ASSERT(
              available_active_approvals.find(id) != available_active_approvals.end() ||
              available_owner_approvals.find(id) != available_owner_approvals.end() ||
              s.check_authority(id) || 
              s.check_authority(get_owner(id)), 
              tx_missing_active_auth,
              "Missing Active Authority ${id}", 
              ("id", id)
              ("auth", get_active(id))
              ("owner", get_owner(id))
            );
        }

        // fetch all of the top level authorities
        for (auto id : required_owner)
        {
            DEIP_ASSERT(
              available_owner_approvals.find(id) != available_owner_approvals.end() ||
              s.check_authority(get_owner(id)),
              tx_missing_owner_auth, 
              "Missing Owner Authority ${id}", 
              ("id", id)
              ("auth", get_owner(id))
            );
        }

        DEIP_ASSERT(
          !s.remove_unused_signatures(), 
          tx_irrelevant_sig, 
          "Unnecessary signature(s) detected"
        );
    }
    FC_CAPTURE_AND_RETHROW((tx_ops)(sigs))
}

flat_set<public_key_type> signed_transaction::get_signature_keys(const chain_id_type& chain_id) const
{
    try
    {
        auto d = sig_digest(chain_id);
        flat_set<public_key_type> result;
        for (const auto& sig : signatures)
        {
            DEIP_ASSERT(result.insert(fc::ecc::public_key(sig, d)).second, tx_duplicate_sig,
                          "Duplicate Signature detected");
        }
        return result;
    }
    FC_CAPTURE_AND_RETHROW()
}

void signed_transaction::verify_tenant_authority(const chain_id_type& chain_id, const authority_getter& get_tenant) const
{
    DEIP_ASSERT(tenant_signature.valid(), tx_missing_tenant_auth, "Missing Tenant Authority"); // required for now
    const auto& val = *tenant_signature;
    auto d = sig_digest(chain_id);
    const public_key_type pub = fc::ecc::public_key(val.signature, d);
    const auto& auth = get_tenant(val.tenant);
    DEIP_ASSERT(auth.key_auths.find(pub) != auth.key_auths.end(), tx_missing_tenant_auth, "Missing Tenant Authority ${id}", ("id", val.tenant));
}

set<public_key_type> signed_transaction::get_required_signatures(const chain_id_type& chain_id,
                                                                 const flat_set<public_key_type>& available_keys,
                                                                 const authority_getter& get_active,
                                                                 const authority_getter& get_owner,
                                                                 const override_authority_getter& get_active_overrides) const
{
    flat_set<account_name_type> required_active;
    flat_set<account_name_type> required_owner;
    vector<authority> other;
    
    get_required_authorities(required_active, required_owner, other);

    sign_state s(get_signature_keys(chain_id), get_active, get_owner, available_keys);

    for (const auto& auth : other)
        s.check_authority(auth);
    for (auto& owner : required_owner)
        s.check_authority(get_owner(owner));
    for (auto& active : required_active)
        s.check_authority(active);

    s.remove_unused_signatures();

    set<public_key_type> result;

    for (auto& provided_sig : s.provided_signatures)
        if (available_keys.find(provided_sig.first) != available_keys.end())
            result.insert(provided_sig.first);

    return result;
}

set<public_key_type>
signed_transaction::minimize_required_signatures(const chain_id_type& chain_id,
                                                 const flat_set<public_key_type>& available_keys,
                                                 const authority_getter& get_active,
                                                 const authority_getter& get_owner,
                                                 const override_authority_getter& get_active_overrides) const
{
    set<public_key_type> s = get_required_signatures(chain_id, available_keys, get_active, get_owner, get_active_overrides);
    flat_set<public_key_type> result(s.begin(), s.end());

    for (const public_key_type& k : s)
    {
        result.erase(k);
        try
        {
            deip::protocol::verify_authority(operations, result, get_active, get_owner, get_active_overrides);
            continue; // element stays erased if verify_authority is ok
        }
        catch (const tx_missing_owner_auth& e)
        {
        }
        catch (const tx_missing_active_auth& e)
        {
        }
        catch (const tx_missing_other_auth& e)
        {
        }
        result.insert(k);
    }
    return set<public_key_type>(result.begin(), result.end());
}

void signed_transaction::verify_authority(const chain_id_type& chain_id,
                                          const authority_getter& get_active,
                                          const authority_getter& get_owner,
                                          const override_authority_getter& get_active_overrides) const
{
    try
    {
        deip::protocol::verify_authority(
            operations, 
            get_signature_keys(chain_id), 
            get_active, 
            get_owner,
            get_active_overrides
        );
    }
    FC_CAPTURE_AND_RETHROW((*this))
}


}
} // deip::protocol
