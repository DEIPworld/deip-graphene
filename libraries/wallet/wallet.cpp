#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>
#include <graphene/utilities/words.hpp>

#include <deip/app/api.hpp>
#include <deip/protocol/base.hpp>
#include <deip/wallet/wallet.hpp>
#include <deip/wallet/api_documentation.hpp>
#include <deip/wallet/reflect_util.hpp>

#include <deip/account_by_key/account_by_key_api.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/container/deque.hpp>
#include <fc/git_revision.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/cli.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/smart_ref_impl.hpp>

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif

#define BRAIN_KEY_WORD_COUNT 16

namespace deip {
namespace wallet {

using deip::wallet::utils::derive_private_key;

namespace detail {

template <class T> optional<T> maybe_id(const string& name_or_id)
{
    if (std::isdigit(name_or_id.front()))
    {
        try
        {
            return fc::variant(name_or_id).as<T>();
        }
        catch (const fc::exception&)
        {
        }
    }
    return optional<T>();
}

string pubkey_to_shorthash(const public_key_type& key)
{
    uint32_t x = fc::sha256::hash(key)._hash[0];
    static const char hd[] = "0123456789abcdef";
    string result;

    result += hd[(x >> 0x1c) & 0x0f];
    result += hd[(x >> 0x18) & 0x0f];
    result += hd[(x >> 0x14) & 0x0f];
    result += hd[(x >> 0x10) & 0x0f];
    result += hd[(x >> 0x0c) & 0x0f];
    result += hd[(x >> 0x08) & 0x0f];
    result += hd[(x >> 0x04) & 0x0f];
    result += hd[(x)&0x0f];

    return result;
}

string normalize_brain_key(const std::string& s)
{
    size_t i = 0, n = s.length();
    std::string result;
    char c;
    result.reserve(n);

    bool preceded_by_whitespace = false;
    bool non_empty = false;
    while (i < n)
    {
        c = s[i++];
        switch (c)
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\v':
        case '\f':
            preceded_by_whitespace = true;
            continue;

        case 'a':
            c = 'A';
            break;
        case 'b':
            c = 'B';
            break;
        case 'c':
            c = 'C';
            break;
        case 'd':
            c = 'D';
            break;
        case 'e':
            c = 'E';
            break;
        case 'f':
            c = 'F';
            break;
        case 'g':
            c = 'G';
            break;
        case 'h':
            c = 'H';
            break;
        case 'i':
            c = 'I';
            break;
        case 'j':
            c = 'J';
            break;
        case 'k':
            c = 'K';
            break;
        case 'l':
            c = 'L';
            break;
        case 'm':
            c = 'M';
            break;
        case 'n':
            c = 'N';
            break;
        case 'o':
            c = 'O';
            break;
        case 'p':
            c = 'P';
            break;
        case 'q':
            c = 'Q';
            break;
        case 'r':
            c = 'R';
            break;
        case 's':
            c = 'S';
            break;
        case 't':
            c = 'T';
            break;
        case 'u':
            c = 'U';
            break;
        case 'v':
            c = 'V';
            break;
        case 'w':
            c = 'W';
            break;
        case 'x':
            c = 'X';
            break;
        case 'y':
            c = 'Y';
            break;
        case 'z':
            c = 'Z';
            break;

        default:
            break;
        }
        if (preceded_by_whitespace && non_empty)
            result.push_back(' ');
        result.push_back(c);
        preceded_by_whitespace = false;
        non_empty = true;
    }
    return result;
}

struct op_prototype_visitor
{
    typedef void result_type;

    int t = 0;
    flat_map<std::string, operation>& name2op;

    op_prototype_visitor(int _t, flat_map<std::string, operation>& _prototype_ops)
        : t(_t)
        , name2op(_prototype_ops)
    {
    }

    template <typename Type> result_type operator()(const Type& op) const
    {
        string name = fc::get_typename<Type>::name();
        size_t p = name.rfind(':');
        if (p != string::npos)
            name = name.substr(p + 1);
        name2op[name] = Type();
    }
};

class wallet_api_impl
{
public:
    api_documentation method_documentation;

private:
    void enable_umask_protection()
    {
#ifdef __unix__
        _old_umask = umask(S_IRWXG | S_IRWXO);
#endif
    }

    void disable_umask_protection()
    {
#ifdef __unix__
        umask(_old_umask);
#endif
    }

    void init_prototype_ops()
    {
        operation op;
        for (int t = 0; t < op.count(); t++)
        {
            op.set_which(t);
            op.visit(op_prototype_visitor(t, _prototype_ops));
        }
        return;
    }

public:
    wallet_api& self;
    wallet_api_impl(wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi)
        : self(s)
        , _chain_id(initial_data.chain_id)
        , _remote_api(rapi)
        , _remote_db(rapi->get_api_by_name("database_api")->as<database_api>())
        , _remote_net_broadcast(rapi->get_api_by_name("network_broadcast_api")->as<network_broadcast_api>())
    {
        init_prototype_ops();

        _wallet.ws_server = initial_data.ws_server;
        _wallet.ws_user = initial_data.ws_user;
        _wallet.ws_password = initial_data.ws_password;
        _wallet.chain_id = initial_data.chain_id;
    }
    virtual ~wallet_api_impl()
    {
    }

    void encrypt_keys()
    {
        if (!is_locked())
        {
            plain_keys data;
            data.keys = _keys;
            data.checksum = _checksum;
            auto plain_txt = fc::raw::pack(data);
            _wallet.cipher_keys = fc::aes_encrypt(data.checksum, plain_txt);
        }
    }

    bool copy_wallet_file(const std::string& destination_filename)
    {
        fc::path src_path = get_wallet_filename();
        if (!fc::exists(src_path))
            return false;
        fc::path dest_path = destination_filename + _wallet_filename_extension;
        int suffix = 0;
        while (fc::exists(dest_path))
        {
            ++suffix;
            dest_path = destination_filename + "-" + std::to_string(suffix) + _wallet_filename_extension;
        }
        wlog("backing up wallet ${src} to ${dest}", ("src", src_path)("dest", dest_path));

        fc::path dest_parent = fc::absolute(dest_path).parent_path();
        try
        {
            enable_umask_protection();
            if (!fc::exists(dest_parent))
                fc::create_directories(dest_parent);
            fc::copy(src_path, dest_path);
            disable_umask_protection();
        }
        catch (...)
        {
            disable_umask_protection();
            throw;
        }
        return true;
    }

    bool is_locked() const
    {
        return _checksum == fc::sha512();
    }

    variant info() const
    {
        auto dynamic_props = _remote_db->get_dynamic_global_properties();
        fc::mutable_variant_object result(fc::variant(dynamic_props).get_object());
        result["witness_majority_version"] = fc::string(_remote_db->get_witness_schedule().majority_version);
        result["hardfork_version"] = fc::string(_remote_db->get_hardfork_version());
        result["head_block_num"] = dynamic_props.head_block_number;
        result["head_block_id"] = dynamic_props.head_block_id;
        result["head_block_age"]
            = fc::get_approximate_relative_time_string(dynamic_props.time, time_point_sec(time_point::now()), " old");
        result["participation"] = (100 * dynamic_props.recent_slots_filled.popcount()) / 128.0;
        result["account_creation_fee"] = _remote_db->get_chain_properties().account_creation_fee;
        result["post_reward_fund"]
            = fc::variant(_remote_db->get_reward_fund(DEIP_POST_REWARD_FUND_NAME)).get_object();
        return result;
    }

    variant_object about() const
    {
        string client_version(graphene::utilities::git_revision_description);
        const size_t pos = client_version.find('/');
        if (pos != string::npos && client_version.size() > pos)
            client_version = client_version.substr(pos + 1);

        fc::mutable_variant_object result;
        result["blockchain_version"] = DEIP_BLOCKCHAIN_VERSION;
        result["client_version"] = client_version;
        result["deip_revision"] = graphene::utilities::git_revision_sha;
        result["deip_revision_age"] = fc::get_approximate_relative_time_string(
            fc::time_point_sec(graphene::utilities::git_revision_unix_timestamp));
        result["fc_revision"] = fc::git_revision_sha;
        result["fc_revision_age"]
            = fc::get_approximate_relative_time_string(fc::time_point_sec(fc::git_revision_unix_timestamp));
        result["compile_date"] = "compiled on " __DATE__ " at " __TIME__;
        result["boost_version"] = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
        result["openssl_version"] = OPENSSL_VERSION_TEXT;

        std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
        std::string os = "osx";
#elif defined(__linux__)
        std::string os = "linux";
#elif defined(_MSC_VER)
        std::string os = "win32";
#else
        std::string os = "other";
#endif
        result["build"] = os + " " + bitness;

        try
        {
            auto v = _remote_api->get_version();
            result["server_blockchain_version"] = v.blockchain_version;
            result["server_deip_revision"] = v.deip_revision;
            result["server_fc_revision"] = v.fc_revision;
        }
        catch (fc::exception&)
        {
            result["server"] = "could not retrieve server version information";
        }

        return result;
    }

    account_api_obj get_account(const std::string& account_name) const
    {
        auto accounts = _remote_db->get_accounts({ account_name });
        FC_ASSERT(!accounts.empty(), "Unknown account");
        return accounts.front();
    }

    string get_wallet_filename() const
    {
        return _wallet_filename;
    }

    optional<fc::ecc::private_key> try_get_private_key(const public_key_type& id) const
    {
        auto it = _keys.find(id);
        if (it != _keys.end())
            return wif_to_key(it->second);
        return optional<fc::ecc::private_key>();
    }

    fc::ecc::private_key get_private_key(const public_key_type& id) const
    {
        auto has_key = try_get_private_key(id);
        FC_ASSERT(has_key);
        return *has_key;
    }

    fc::ecc::private_key get_private_key_for_account(const account_api_obj& account) const
    {
        vector<public_key_type> active_keys = account.active.get_keys();
        if (active_keys.size() != 1)
            FC_THROW("Expecting a simple authority with one active key");
        return get_private_key(active_keys.front());
    }

    // imports the private key into the wallet, and associate it in some way (?) with the
    // given account name.
    // @returns true if the key matches a current active/owner/memo key for the named
    //          account, false otherwise (but it is stored either way)
    bool import_key(const std::string& wif_key)
    {
        fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
        if (!optional_private_key)
            FC_THROW("Invalid private key");
        deip::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

        _keys[wif_pub_key] = wif_key;
        return true;
    }

    // TODO: Needs refactoring
    bool load_wallet_file(string wallet_filename = "")
    {
        // TODO:  Merge imported wallet with existing wallet,
        //        instead of replacing it
        if (wallet_filename == "")
            wallet_filename = _wallet_filename;

        if (!fc::exists(wallet_filename))
            return false;

        _wallet = fc::json::from_file(wallet_filename).as<wallet_data>();

        return true;
    }

    // TODO: Needs refactoring
    void save_wallet_file(string wallet_filename = "")
    {
        //
        // Serialize in memory, then save to disk
        //
        // This approach lessens the risk of a partially written wallet
        // if exceptions are thrown in serialization
        //

        encrypt_keys();

        if (wallet_filename == "")
            wallet_filename = _wallet_filename;

        wlog("saving wallet to file ${fn}", ("fn", wallet_filename));

        string data = fc::json::to_pretty_string(_wallet);
        try
        {
            enable_umask_protection();
            //
            // Parentheses on the following declaration fails to compile,
            // due to the Most Vexing Parse.  Thanks, C++
            //
            // http://en.wikipedia.org/wiki/Most_vexing_parse
            //
            fc::ofstream outfile{ fc::path(wallet_filename) };
            outfile.write(data.c_str(), data.length());
            outfile.flush();
            outfile.close();
            disable_umask_protection();
        }
        catch (...)
        {
            disable_umask_protection();
            throw;
        }
    }

    // This function generates derived keys starting with index 0 and keeps incrementing
    // the index until it finds a key that isn't registered in the block chain.  To be
    // safer, it continues checking for a few more keys to make sure there wasn't a short gap
    // caused by a failed registration or the like.
    int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
    {
        int first_unused_index = 0;
        int number_of_consecutive_unused_keys = 0;
        for (int key_index = 0;; ++key_index)
        {
            fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
            deip::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
            if (_keys.find(derived_public_key) == _keys.end())
            {
                if (number_of_consecutive_unused_keys)
                {
                    ++number_of_consecutive_unused_keys;
                    if (number_of_consecutive_unused_keys > 5)
                        return first_unused_index;
                }
                else
                {
                    first_unused_index = key_index;
                    number_of_consecutive_unused_keys = 1;
                }
            }
            else
            {
                // key_index is used
                first_unused_index = 0;
                number_of_consecutive_unused_keys = 0;
            }
        }
    }

    signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                       const std::string& account_name,
                                                       const std::string& creator_account_name,
                                                       bool broadcast = false,
                                                       bool save_wallet = true)
    {
        try
        {
            int active_key_index = find_first_unused_derived_key_index(owner_privkey);
            fc::ecc::private_key active_privkey = derive_private_key(key_to_wif(owner_privkey), active_key_index);

            int memo_key_index = find_first_unused_derived_key_index(active_privkey);
            fc::ecc::private_key memo_privkey = derive_private_key(key_to_wif(active_privkey), memo_key_index);

            deip::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
            deip::chain::public_key_type active_pubkey = active_privkey.get_public_key();
            deip::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

            account_create_operation account_create_op;

            account_create_op.creator = creator_account_name;
            account_create_op.new_account_name = account_name;
            account_create_op.fee = _remote_db->get_chain_properties().account_creation_fee;
            account_create_op.owner = authority(1, owner_pubkey, 1);
            account_create_op.active = authority(1, active_pubkey, 1);
            account_create_op.memo_key = memo_pubkey;

            signed_transaction tx;

            tx.operations.push_back(account_create_op);
            tx.validate();

            if (save_wallet)
                save_wallet_file();
            if (broadcast)
            {
                //_remote_net_broadcast->broadcast_transaction( tx );
                auto result = _remote_net_broadcast->broadcast_transaction_synchronous(tx);
            }
            return tx;
        }
        FC_CAPTURE_AND_RETHROW((account_name)(creator_account_name)(broadcast))
    }

    signed_transaction
    set_voting_proxy(const std::string& account_to_modify, const std::string& proxy, bool broadcast /* = false */)
    {
        try
        {
            account_witness_proxy_operation op;
            op.account = account_to_modify;
            op.proxy = proxy;

            signed_transaction tx;
            tx.operations.push_back(op);
            tx.validate();

            return sign_transaction(tx, broadcast);
        }
        FC_CAPTURE_AND_RETHROW((account_to_modify)(proxy)(broadcast))
    }

    optional<witness_api_obj> get_witness(const std::string& owner_account)
    {
        return _remote_db->get_witness_by_account(owner_account);
    }

    void set_transaction_expiration(uint32_t tx_expiration_seconds)
    {
        FC_ASSERT(tx_expiration_seconds < DEIP_MAX_TIME_UNTIL_EXPIRATION);
        _tx_expiration_seconds = tx_expiration_seconds;
    }

    annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
    {
        flat_set<account_name_type> req_active_approvals;
        flat_set<account_name_type> req_owner_approvals;
        flat_set<account_name_type> req_posting_approvals;
        vector<authority> other_auths;

        tx.get_required_authorities(req_active_approvals, req_owner_approvals, req_posting_approvals, other_auths);

        for (const auto& auth : other_auths)
            for (const auto& a : auth.account_auths)
                req_active_approvals.insert(a.first);

        // std::merge lets us de-duplicate account_id's that occur in both
        //   sets, and dump them into a vector (as required by remote_db api)
        //   at the same time
        vector<string> v_approving_account_names;
        std::merge(req_active_approvals.begin(), req_active_approvals.end(), req_owner_approvals.begin(),
                   req_owner_approvals.end(), std::back_inserter(v_approving_account_names));

        for (const auto& a : req_posting_approvals)
            v_approving_account_names.push_back(a);

        /// TODO: fetch the accounts specified via other_auths as well.

        auto approving_account_objects = _remote_db->get_accounts(v_approving_account_names);

        /// TODO: recursively check one layer deeper in the authority tree for keys

        FC_ASSERT(approving_account_objects.size() == v_approving_account_names.size(), "",
                  ("aco.size:", approving_account_objects.size())("acn", v_approving_account_names.size()));

        flat_map<string, account_api_obj> approving_account_lut;
        size_t i = 0;
        for (const optional<account_api_obj>& approving_acct : approving_account_objects)
        {
            if (!approving_acct.valid())
            {
                wlog("operation_get_required_auths said approval of non-existing account ${name} was needed",
                     ("name", v_approving_account_names[i]));
                i++;
                continue;
            }
            approving_account_lut[approving_acct->name] = *approving_acct;
            i++;
        }

        flat_set<public_key_type> approving_key_set;
        for (account_name_type& acct_name : req_active_approvals)
        {
            const auto it = approving_account_lut.find(acct_name);
            if (it == approving_account_lut.end())
                continue;
            const account_api_obj& acct = it->second;
            vector<public_key_type> v_approving_keys = acct.active.get_keys();
            wdump((v_approving_keys));
            for (const public_key_type& approving_key : v_approving_keys)
            {
                wdump((approving_key));
                approving_key_set.insert(approving_key);
            }
        }

        for (account_name_type& acct_name : req_posting_approvals)
        {
            const auto it = approving_account_lut.find(acct_name);
            if (it == approving_account_lut.end())
                continue;
            const account_api_obj& acct = it->second;
            vector<public_key_type> v_approving_keys = acct.posting.get_keys();
            wdump((v_approving_keys));
            for (const public_key_type& approving_key : v_approving_keys)
            {
                wdump((approving_key));
                approving_key_set.insert(approving_key);
            }
        }

        for (const account_name_type& acct_name : req_owner_approvals)
        {
            const auto it = approving_account_lut.find(acct_name);
            if (it == approving_account_lut.end())
                continue;
            const account_api_obj& acct = it->second;
            vector<public_key_type> v_approving_keys = acct.owner.get_keys();
            for (const public_key_type& approving_key : v_approving_keys)
            {
                wdump((approving_key));
                approving_key_set.insert(approving_key);
            }
        }
        for (const authority& a : other_auths)
        {
            for (const auto& k : a.key_auths)
            {
                wdump((k.first));
                approving_key_set.insert(k.first);
            }
        }

        auto dyn_props = _remote_db->get_dynamic_global_properties();
        tx.set_reference_block(dyn_props.head_block_id);
        tx.set_expiration(dyn_props.time + fc::seconds(_tx_expiration_seconds));
        tx.signatures.clear();

        // idump((_keys));
        flat_set<public_key_type> available_keys;
        flat_map<public_key_type, fc::ecc::private_key> available_private_keys;
        for (const public_key_type& key : approving_key_set)
        {
            auto it = _keys.find(key);
            if (it != _keys.end())
            {
                fc::optional<fc::ecc::private_key> privkey = wif_to_key(it->second);
                FC_ASSERT(privkey.valid(), "Malformed private key in _keys");
                available_keys.insert(key);
                available_private_keys[key] = *privkey;
            }
        }

        auto get_account_from_lut = [&](const std::string& name) -> const account_api_obj& {
            auto it = approving_account_lut.find(name);
            FC_ASSERT(it != approving_account_lut.end());
            return it->second;
        };

        auto minimal_signing_keys = tx.minimize_required_signatures(
            _chain_id, available_keys,
            [&](const string& account_name) -> const authority& { return (get_account_from_lut(account_name).active); },
            [&](const string& account_name) -> const authority& { return (get_account_from_lut(account_name).owner); },
            [&](const string& account_name) -> const authority& {
                return (get_account_from_lut(account_name).posting);
            },
            DEIP_MAX_SIG_CHECK_DEPTH);

        for (const public_key_type& k : minimal_signing_keys)
        {
            auto it = available_private_keys.find(k);
            FC_ASSERT(it != available_private_keys.end());
            tx.sign(it->second, _chain_id);
        }

        if (broadcast)
        {
            try
            {
                auto result = _remote_net_broadcast->broadcast_transaction_synchronous(tx);
                annotated_signed_transaction rtrx(tx);
                rtrx.block_num = result.get_object()["block_num"].as_uint64();
                rtrx.transaction_num = result.get_object()["trx_num"].as_uint64();
                return rtrx;
            }
            catch (const fc::exception& e)
            {
                elog("Caught exception while broadcasting tx ${id}:  ${e}",
                     ("id", tx.id().str())("e", e.to_detail_string()));
                throw;
            }
        }
        return tx;
    }

    std::map<string, std::function<string(fc::variant, const fc::variants&)>> get_result_formatters() const
    {
        std::map<string, std::function<string(fc::variant, const fc::variants&)>> m;
        m["help"] = [](variant result, const fc::variants& a) { return result.get_string(); };

        m["gethelp"] = [](variant result, const fc::variants& a) { return result.get_string(); };

        m["list_my_accounts"] = [](variant result, const fc::variants& a) {
            std::stringstream out;

            auto accounts = result.as<vector<account_api_obj>>();
            asset total_deip;
            share_type total_common_tokens_amount;
            share_type total_expert_tokens_amount;
            for (const auto& a : accounts)
            {
                total_deip += a.balance;
                total_common_tokens_amount += a.common_tokens_balance;
                total_expert_tokens_amount += a.expert_tokens_balance;
                out << std::left << std::setw(17) << std::string(a.name) << std::right << std::setw(18)
                    << fc::variant(a.balance).as_string() << " " << std::right << std::setw(26)
                    << std::to_string(total_common_tokens_amount.value)<< " " << std::right << std::setw(28)
                    << std::to_string(total_expert_tokens_amount.value) << "\n";
            }
            out << "-------------------------------------------------------------------------\n";
            out << std::left << std::setw(17) << "TOTAL" << std::right << std::setw(18)
                << fc::variant(total_deip).as_string() << " " << std::right << std::setw(26)
                    << std::to_string(total_common_tokens_amount.value)<< " " << std::right << std::setw(28)
                    << std::to_string(total_expert_tokens_amount.value) << "\n";
            return out.str();
        };
        m["get_account_history"] = [](variant result, const fc::variants& a) {
            std::stringstream ss;
            ss << std::left << std::setw(5) << "#"
               << " ";
            ss << std::left << std::setw(10) << "BLOCK #"
               << " ";
            ss << std::left << std::setw(15) << "TRX ID"
               << " ";
            ss << std::left << std::setw(20) << "OPERATION"
               << " ";
            ss << std::left << std::setw(50) << "DETAILS"
               << "\n";
            ss << "-------------------------------------------------------------------------------\n";
            const auto& results = result.get_array();
            for (const auto& item : results)
            {
                ss << std::left << std::setw(5) << item.get_array()[0].as_string() << " ";
                const auto& op = item.get_array()[1].get_object();
                ss << std::left << std::setw(10) << op["block"].as_string() << " ";
                ss << std::left << std::setw(15) << op["trx_id"].as_string() << " ";
                const auto& opop = op["op"].get_array();
                ss << std::left << std::setw(20) << opop[0].as_string() << " ";
                ss << std::left << std::setw(50) << fc::json::to_string(opop[1]) << "\n ";
            }
            return ss.str();
        };
        m["get_withdraw_routes"] = [](variant result, const fc::variants& a) {
            auto routes = result.as<vector<withdraw_route>>();
            std::stringstream ss;

            ss << ' ' << std::left << std::setw(20) << "From";
            ss << ' ' << std::left << std::setw(20) << "To";
            ss << ' ' << std::right << std::setw(8) << "Percent";
            ss << ' ' << std::right << std::setw(9) << "Auto-Vest";
            ss << "\n==============================================================\n";

            for (auto r : routes)
            {
                ss << ' ' << std::left << std::setw(20) << r.from_account;
                ss << ' ' << std::left << std::setw(20) << r.to_account;
                ss << ' ' << std::right << std::setw(8) << std::setprecision(2) << std::fixed
                   << double(r.percent) / 100;
                ss << ' ' << std::right << std::setw(9) << (r.auto_common_token ? "true" : "false") << std::endl;
            }

            return ss.str();
        };

        return m;
    }

    void use_network_node_api()
    {
        if (_remote_net_node)
            return;
        try
        {
            _remote_net_node = _remote_api->get_api_by_name("network_node_api")->as<network_node_api>();
        }
        catch (const fc::exception& e)
        {
            elog("Couldn't get network node API");
            throw(e);
        }
    }

    void use_remote_account_by_key_api()
    {
        if (_remote_account_by_key_api.valid())
            return;

        try
        {
            _remote_account_by_key_api
                = _remote_api->get_api_by_name("account_by_key_api")->as<account_by_key::account_by_key_api>();
        }
        catch (const fc::exception& e)
        {
            elog("Couldn't get account_by_key API");
            throw(e);
        }
    }

    void network_add_nodes(const vector<string>& nodes)
    {
        use_network_node_api();
        for (const string& node_address : nodes)
        {
            (*_remote_net_node)->add_node(fc::ip::endpoint::from_string(node_address));
        }
    }

    vector<variant> network_get_connected_peers()
    {
        use_network_node_api();
        const auto peers = (*_remote_net_node)->get_connected_peers();
        vector<variant> result;
        result.reserve(peers.size());
        for (const auto& peer : peers)
        {
            variant v;
            fc::to_variant(peer, v);
            result.push_back(v);
        }
        return result;
    }

    operation get_prototype_operation(const std::string& operation_name)
    {
        auto it = _prototype_ops.find(operation_name);
        if (it == _prototype_ops.end())
            FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
        return it->second;
    }

    string _wallet_filename;
    wallet_data _wallet;

    map<public_key_type, string> _keys;
    fc::sha512 _checksum;

    chain_id_type _chain_id;

    fc::api<login_api> _remote_api;
    fc::api<database_api> _remote_db;
    fc::api<network_broadcast_api> _remote_net_broadcast;
    optional<fc::api<network_node_api>> _remote_net_node;
    optional<fc::api<account_by_key::account_by_key_api>> _remote_account_by_key_api;
    uint32_t _tx_expiration_seconds = 30;

    flat_map<string, operation> _prototype_ops;

    static_variant_map _operation_which_map = create_static_variant_map<operation>();

#ifdef __unix__
    mode_t _old_umask;
#endif
    const string _wallet_filename_extension = ".wallet";
};

} // namespace detail

wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
    : my(new detail::wallet_api_impl(*this, initial_data, rapi))
{
}

wallet_api::~wallet_api()
{
}

bool wallet_api::copy_wallet_file(const string& destination_filename)
{
    return my->copy_wallet_file(destination_filename);
}

optional<signed_block_api_obj> wallet_api::get_block(uint32_t num)
{
    return my->_remote_db->get_block(num);
}

vector<applied_operation> wallet_api::get_ops_in_block(uint32_t block_num, bool only_virtual)
{
    return my->_remote_db->get_ops_in_block(block_num, only_virtual);
}

vector<account_api_obj> wallet_api::list_my_accounts()
{
    FC_ASSERT(!is_locked(), "Wallet must be unlocked to list accounts");
    vector<account_api_obj> result;

    try
    {
        my->use_remote_account_by_key_api();
    }
    catch (fc::exception& e)
    {
        elog("Connected node needs to enable account_by_key_api");
        return result;
    }

    vector<public_key_type> pub_keys;
    pub_keys.reserve(my->_keys.size());

    for (const auto& item : my->_keys)
        pub_keys.push_back(item.first);

    auto refs = (*my->_remote_account_by_key_api)->get_key_references(pub_keys);
    set<string> names;
    for (const auto& item : refs)
        for (const auto& name : item)
            names.insert(name);

    result.reserve(names.size());
    for (const auto& name : names)
        result.emplace_back(get_account(name));

    return result;
}

set<string> wallet_api::list_accounts(const string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_accounts(lowerbound, limit);
}

std::vector<account_name_type> wallet_api::get_active_witnesses() const
{
    return my->_remote_db->get_active_witnesses();
}

string wallet_api::serialize_transaction(const signed_transaction& tx) const
{
    return fc::to_hex(fc::raw::pack(tx));
}

string wallet_api::get_wallet_filename() const
{
    return my->get_wallet_filename();
}

account_api_obj wallet_api::get_account(const std::string& account_name) const
{
    return my->get_account(account_name);
}

bool wallet_api::import_key(const std::string& wif_key)
{
    FC_ASSERT(!is_locked());
    // backup wallet
    fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
    if (!optional_private_key)
        FC_THROW("Invalid private key");
    //   string shorthash = detail::pubkey_to_shorthash( optional_private_key->get_public_key() );
    //   copy_wallet_file( "before-import-key-" + shorthash );

    if (my->import_key(wif_key))
    {
        save_wallet_file();
        //     copy_wallet_file( "after-import-key-" + shorthash );
        return true;
    }
    return false;
}

string wallet_api::normalize_brain_key(const std::string& s) const
{
    return detail::normalize_brain_key(s);
}

variant wallet_api::info()
{
    return my->info();
}

variant_object wallet_api::about() const
{
    return my->about();
}

set<account_name_type> wallet_api::list_witnesses(const string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_witness_accounts(lowerbound, limit);
}

optional<witness_api_obj> wallet_api::get_witness(const std::string& owner_account)
{
    return my->get_witness(owner_account);
}

annotated_signed_transaction wallet_api::set_voting_proxy(const std::string& account_to_modify,
                                                          const std::string& voting_account,
                                                          bool broadcast /* = false */)
{
    return my->set_voting_proxy(account_to_modify, voting_account, broadcast);
}

void wallet_api::set_wallet_filename(const std::string& wallet_filename)
{
    my->_wallet_filename = wallet_filename;
}

brain_key_info wallet_api::suggest_brain_key() const
{
    return utils::suggest_brain_key();
}

annotated_signed_transaction wallet_api::sign_transaction(const signed_transaction& tx, bool broadcast /* = false */)
{
    try
    {
        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((tx))
}

operation wallet_api::get_prototype_operation(const std::string& operation_name)
{
    return my->get_prototype_operation(operation_name);
}

void wallet_api::network_add_nodes(const vector<string>& nodes)
{
    my->network_add_nodes(nodes);
}

vector<variant> wallet_api::network_get_connected_peers()
{
    return my->network_get_connected_peers();
}

string wallet_api::help() const
{
    std::vector<std::string> method_names = my->method_documentation.get_method_names();
    std::stringstream ss;
    for (const std::string method_name : method_names)
    {
        try
        {
            ss << my->method_documentation.get_brief_description(method_name);
        }
        catch (const fc::key_not_found_exception&)
        {
            ss << method_name << " (no help available)\n";
        }
    }
    return ss.str();
}

string wallet_api::gethelp(const std::string& method) const
{
    fc::api<wallet_api> tmp;
    std::stringstream ss;
    ss << "\n";

    std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
    if (!doxygenHelpString.empty())
        ss << doxygenHelpString;
    else
        ss << "No help defined for method " << method << "\n";

    return ss.str();
}

bool wallet_api::load_wallet_file(const std::string& wallet_filename)
{
    return my->load_wallet_file(wallet_filename);
}

void wallet_api::save_wallet_file(const std::string& wallet_filename)
{
    my->save_wallet_file(wallet_filename);
}

std::map<string, std::function<string(fc::variant, const fc::variants&)>> wallet_api::get_result_formatters() const
{
    return my->get_result_formatters();
}

bool wallet_api::is_locked() const
{
    return my->is_locked();
}
bool wallet_api::is_new() const
{
    return my->_wallet.cipher_keys.size() == 0;
}

void wallet_api::encrypt_keys()
{
    my->encrypt_keys();
}

void wallet_api::lock()
{
    try
    {
        FC_ASSERT(!is_locked());
        encrypt_keys();
        for (auto key : my->_keys)
            key.second = key_to_wif(fc::ecc::private_key());
        my->_keys.clear();
        my->_checksum = fc::sha512();
        my->self.lock_changed(true);
    }
    FC_CAPTURE_AND_RETHROW()
}

void wallet_api::unlock(const string& password)
{
    try
    {
        FC_ASSERT(password.size() > 0);
        auto pw = fc::sha512::hash(password.c_str(), password.size());
        vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
        auto pk = fc::raw::unpack<plain_keys>(decrypted);
        FC_ASSERT(pk.checksum == pw);
        my->_keys = std::move(pk.keys);
        my->_checksum = pk.checksum;
        my->self.lock_changed(false);
    }
    FC_CAPTURE_AND_RETHROW()
}

void wallet_api::set_password(const std::string& password)
{
    if (!is_new())
        FC_ASSERT(!is_locked(), "The wallet must be unlocked before the password can be set");
    my->_checksum = fc::sha512::hash(password.c_str(), password.size());
    lock();
}

map<public_key_type, string> wallet_api::list_keys()
{
    FC_ASSERT(!is_locked());
    return my->_keys;
}

string wallet_api::get_private_key(const public_key_type& pubkey) const
{
    return key_to_wif(my->get_private_key(pubkey));
}

pair<public_key_type, string> wallet_api::get_private_key_from_password(const std::string& account,
                                                                        const std::string& role,
                                                                        const std::string& password) const
{
    auto seed = account + role + password;
    FC_ASSERT(seed.size());
    auto secret = fc::sha256::hash(seed.c_str(), seed.size());
    auto priv = fc::ecc::private_key::regenerate(secret);
    return std::make_pair(public_key_type(priv.get_public_key()), key_to_wif(priv));
}

/**
 * This method is used by faucets to create new accounts for other users which must
 * provide their desired keys. The resulting account may not be controllable by this
 * wallet.
 */
annotated_signed_transaction wallet_api::create_account_with_keys(const std::string& creator,
                                                                  const std::string& newname,
                                                                  const std::string& json_meta,
                                                                  const public_key_type& owner,
                                                                  const public_key_type& active,
                                                                  const public_key_type& posting,
                                                                  const public_key_type& memo,
                                                                  bool broadcast) const
{
    try
    {
        FC_ASSERT(!is_locked());
        account_create_operation op;
        op.creator = creator;
        op.new_account_name = newname;
        op.owner = authority(1, owner, 1);
        op.active = authority(1, active, 1);
        op.posting = authority(1, posting, 1);
        op.memo_key = memo;
        op.json_metadata = json_meta;
        op.fee = my->_remote_db->get_chain_properties().account_creation_fee
            * asset(DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER, DEIP_SYMBOL);

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((creator)(newname)(json_meta)(owner)(active)(memo)(broadcast))
}

annotated_signed_transaction wallet_api::request_account_recovery(const std::string& recovery_account,
                                                                  const std::string& account_to_recover,
                                                                  const authority& new_authority,
                                                                  bool broadcast)
{
    FC_ASSERT(!is_locked());
    request_account_recovery_operation op;
    op.recovery_account = recovery_account;
    op.account_to_recover = account_to_recover;
    op.new_owner_authority = new_authority;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::recover_account(const std::string& account_to_recover,
                                                         const authority& recent_authority,
                                                         const authority& new_authority,
                                                         bool broadcast)
{
    FC_ASSERT(!is_locked());

    recover_account_operation op;
    op.account_to_recover = account_to_recover;
    op.new_owner_authority = new_authority;
    op.recent_owner_authority = recent_authority;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::change_recovery_account(const std::string& owner, const std::string& new_recovery_account, bool broadcast)
{
    FC_ASSERT(!is_locked());

    change_recovery_account_operation op;
    op.account_to_recover = owner;
    op.new_recovery_account = new_recovery_account;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

vector<owner_authority_history_api_obj> wallet_api::get_owner_history(const std::string& account) const
{
    return my->_remote_db->get_owner_history(account);
}

annotated_signed_transaction wallet_api::update_account(const std::string& account_name,
                                                        const std::string& json_meta,
                                                        const public_key_type& owner,
                                                        const public_key_type& active,
                                                        const public_key_type& posting,
                                                        const public_key_type& memo,
                                                        bool broadcast) const
{
    try
    {
        FC_ASSERT(!is_locked());

        account_update_operation op;
        op.account = account_name;
        op.owner = authority(1, owner, 1);
        op.active = authority(1, active, 1);
        op.posting = authority(1, posting, 1);
        op.memo_key = memo;
        op.json_metadata = json_meta;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((account_name)(json_meta)(owner)(active)(memo)(broadcast))
}

annotated_signed_transaction wallet_api::update_account_auth_key(const std::string& account_name,
                                                                 const authority_type& type,
                                                                 const public_key_type& key,
                                                                 weight_type weight,
                                                                 bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = accounts[0].memo_key;
    op.json_metadata = accounts[0].json_metadata;

    authority new_auth;

    switch (type)
    {
    case (owner):
        new_auth = accounts[0].owner;
        break;
    case (active):
        new_auth = accounts[0].active;
        break;
    case (posting):
        new_auth = accounts[0].posting;
        break;
    }

    if (weight == 0) // Remove the key
    {
        new_auth.key_auths.erase(key);
    }
    else
    {
        new_auth.add_authority(key, weight);
    }

    if (new_auth.is_impossible())
    {
        if (type == owner)
        {
            FC_ASSERT(false, "Owner authority change would render account irrecoverable.");
        }

        wlog("Authority is now impossible.");
    }

    switch (type)
    {
    case (owner):
        op.owner = new_auth;
        break;
    case (active):
        op.active = new_auth;
        break;
    case (posting):
        op.posting = new_auth;
        break;
    }

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::update_account_auth_account(const std::string& account_name,
                                                                     authority_type type,
                                                                     const std::string& auth_account,
                                                                     weight_type weight,
                                                                     bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = accounts[0].memo_key;
    op.json_metadata = accounts[0].json_metadata;

    authority new_auth;

    switch (type)
    {
    case (owner):
        new_auth = accounts[0].owner;
        break;
    case (active):
        new_auth = accounts[0].active;
        break;
    case (posting):
        new_auth = accounts[0].posting;
        break;
    }

    if (weight == 0) // Remove the key
    {
        new_auth.account_auths.erase(auth_account);
    }
    else
    {
        new_auth.add_authority(auth_account, weight);
    }

    if (new_auth.is_impossible())
    {
        if (type == owner)
        {
            FC_ASSERT(false, "Owner authority change would render account irrecoverable.");
        }

        wlog("Authority is now impossible.");
    }

    switch (type)
    {
    case (owner):
        op.owner = new_auth;
        break;
    case (active):
        op.active = new_auth;
        break;
    case (posting):
        op.posting = new_auth;
        break;
    }

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::update_account_auth_threshold(const std::string& account_name,
                                                                       authority_type type,
                                                                       uint32_t threshold,
                                                                       bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");
    FC_ASSERT(threshold != 0, "Authority is implicitly satisfied");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = accounts[0].memo_key;
    op.json_metadata = accounts[0].json_metadata;

    authority new_auth;

    switch (type)
    {
    case (owner):
        new_auth = accounts[0].owner;
        break;
    case (active):
        new_auth = accounts[0].active;
        break;
    case (posting):
        new_auth = accounts[0].posting;
        break;
    }

    new_auth.weight_threshold = threshold;

    if (new_auth.is_impossible())
    {
        if (type == owner)
        {
            FC_ASSERT(false, "Owner authority change would render account irrecoverable.");
        }

        wlog("Authority is now impossible.");
    }

    switch (type)
    {
    case (owner):
        op.owner = new_auth;
        break;
    case (active):
        op.active = new_auth;
        break;
    case (posting):
        op.posting = new_auth;
        break;
    }

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::update_account_meta(const std::string& account_name, const std::string& json_meta, bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = accounts[0].memo_key;
    op.json_metadata = json_meta;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::update_account_memo_key(const std::string& account_name, const public_key_type& key, bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = key;
    op.json_metadata = accounts[0].json_metadata;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

/**
 *  This method will genrate new owner, active, and memo keys for the new account which
 *  will be controlable by this wallet.
 */
annotated_signed_transaction wallet_api::create_account(const std::string& creator,
                                                        const std::string& newname,
                                                        const std::string& json_meta,
                                                        bool broadcast)
{
    try
    {
        FC_ASSERT(!is_locked());
        auto owner = suggest_brain_key();
        auto active = suggest_brain_key();
        auto posting = suggest_brain_key();
        auto memo = suggest_brain_key();
        import_key(owner.wif_priv_key);
        import_key(active.wif_priv_key);
        import_key(posting.wif_priv_key);
        import_key(memo.wif_priv_key);
        return create_account_with_keys(creator, newname, json_meta, owner.pub_key, active.pub_key, posting.pub_key,
                                        memo.pub_key, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((creator)(newname)(json_meta))
}

annotated_signed_transaction wallet_api::update_witness(const std::string& witness_account_name,
                                                        const std::string& url,
                                                        const public_key_type& block_signing_key,
                                                        const chain_properties& props,
                                                        bool broadcast)
{
    FC_ASSERT(!is_locked());

    witness_update_operation op;

    fc::optional<witness_api_obj> wit = my->_remote_db->get_witness_by_account(witness_account_name);
    if (!wit.valid())
    {
        op.url = url;
    }
    else
    {
        FC_ASSERT(wit->owner == witness_account_name);
        if (url != "")
            op.url = url;
        else
            op.url = wit->url;
    }
    op.owner = witness_account_name;
    op.block_signing_key = block_signing_key;
    op.props = props;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::vote_for_witness(const std::string& voting_account,
                                                          const std::string& witness_to_vote_for,
                                                          bool approve,
                                                          bool broadcast)
{
    try
    {
        FC_ASSERT(!is_locked());
        account_witness_vote_operation op;
        op.account = voting_account;
        op.witness = witness_to_vote_for;
        op.approve = approve;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((voting_account)(witness_to_vote_for)(approve)(broadcast))
}

void wallet_api::check_memo(const string& memo, const account_api_obj& account) const
{
    vector<public_key_type> keys;

    try
    {
        // Check if memo is a private key
        keys.push_back(fc::ecc::extended_private_key::from_base58(memo).get_public_key());
    }
    catch (fc::parse_error_exception&)
    {
    }
    catch (fc::assert_exception&)
    {
    }

    // Get possible keys if memo was an account password
    string owner_seed = account.name + "owner" + memo;
    auto owner_secret = fc::sha256::hash(owner_seed.c_str(), owner_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(owner_secret).get_public_key());

    string active_seed = account.name + "active" + memo;
    auto active_secret = fc::sha256::hash(active_seed.c_str(), active_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(active_secret).get_public_key());

    string posting_seed = account.name + "posting" + memo;
    auto posting_secret = fc::sha256::hash(posting_seed.c_str(), posting_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(posting_secret).get_public_key());

    // Check keys against public keys in authorites
    for (auto& key_weight_pair : account.owner.key_auths)
    {
        for (auto& key : keys)
            FC_ASSERT(key_weight_pair.first != key,
                      "Detected private owner key in memo field. Cancelling transaction.");
    }

    for (auto& key_weight_pair : account.active.key_auths)
    {
        for (auto& key : keys)
            FC_ASSERT(key_weight_pair.first != key,
                      "Detected private active key in memo field. Cancelling transaction.");
    }

    for (auto& key_weight_pair : account.posting.key_auths)
    {
        for (auto& key : keys)
            FC_ASSERT(key_weight_pair.first != key,
                      "Detected private posting key in memo field. Cancelling transaction.");
    }

    const auto& memo_key = account.memo_key;
    for (auto& key : keys)
        FC_ASSERT(memo_key != key, "Detected private memo key in memo field. Cancelling transaction.");

    // Check against imported keys
    for (auto& key_pair : my->_keys)
    {
        for (auto& key : keys)
            FC_ASSERT(key != key_pair.first, "Detected imported private key in memo field. Cancelling trasanction.");
    }
}

string wallet_api::get_encrypted_memo(const std::string& from, const std::string& to, const std::string& memo)
{

    if (memo.size() > 0 && memo[0] == '#')
    {
        memo_data m;

        auto from_account = get_account(from);
        auto to_account = get_account(to);

        m.from = from_account.memo_key;
        m.to = to_account.memo_key;
        m.nonce = fc::time_point::now().time_since_epoch().count();

        auto from_priv = my->get_private_key(m.from);
        auto shared_secret = from_priv.get_shared_secret(m.to);

        fc::sha512::encoder enc;
        fc::raw::pack(enc, m.nonce);
        fc::raw::pack(enc, shared_secret);
        auto encrypt_key = enc.result();

        m.encrypted = fc::aes_encrypt(encrypt_key, fc::raw::pack(memo.substr(1)));
        m.check = fc::sha256::hash(encrypt_key)._hash[0];
        return m;
    }
    else
    {
        return memo;
    }
}

annotated_signed_transaction wallet_api::transfer(
    const std::string& from, const std::string& to, const asset& amount, const std::string& memo, bool broadcast)
{
    try
    {
        FC_ASSERT(!is_locked());
        check_memo(memo, get_account(from));
        transfer_operation op;
        op.from = from;
        op.to = to;
        op.amount = amount;

        op.memo = get_encrypted_memo(from, to, memo);

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((from)(to)(amount)(memo)(broadcast))
}

annotated_signed_transaction
wallet_api::transfer_to_common_tokens(const std::string& from, const std::string& to, const asset& amount, bool broadcast)
{
    FC_ASSERT(!is_locked());
    transfer_to_common_tokens_operation op;
    op.from = from;
    op.to = (to == from ? "" : to);
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::withdraw_common_tokens(const std::string& from, const share_type& common_tokens_amount, bool broadcast)
{
    FC_ASSERT(!is_locked());
    withdraw_common_tokens_operation op;
    op.account = from;
    op.total_common_tokens_amount = common_tokens_amount;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::set_withdraw_common_tokens_route(
    const std::string& from, const std::string& to, uint16_t percent, bool auto_common_token, bool broadcast)
{
    FC_ASSERT(!is_locked());
    set_withdraw_common_tokens_route_operation op;
    op.from_account = from;
    op.to_account = to;
    op.percent = percent;
    op.auto_common_token = auto_common_token;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::transfer_research_tokens(const int64_t research_token_id, const int64_t research_id, const std::string& from, const std::string& to, const uint32_t amount, bool broadcast)
{
    FC_ASSERT(!is_locked());
    transfer_research_tokens_operation op;
    op.research_token_id = research_token_id;
    op.research_id = research_id;
    op.sender = from;
    op.receiver = to;
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

string wallet_api::decrypt_memo(const std::string& encrypted_memo)
{
    if (is_locked())
        return encrypted_memo;

    if (encrypted_memo.size() && encrypted_memo[0] == '#')
    {
        auto m = memo_data::from_string(encrypted_memo);
        if (m)
        {
            fc::sha512 shared_secret;
            auto from_key = my->try_get_private_key(m->from);
            if (!from_key)
            {
                auto to_key = my->try_get_private_key(m->to);
                if (!to_key)
                    return encrypted_memo;
                shared_secret = to_key->get_shared_secret(m->from);
            }
            else
            {
                shared_secret = from_key->get_shared_secret(m->to);
            }
            fc::sha512::encoder enc;
            fc::raw::pack(enc, m->nonce);
            fc::raw::pack(enc, shared_secret);
            auto encryption_key = enc.result();

            uint32_t check = fc::sha256::hash(encryption_key)._hash[0];
            if (check != m->check)
                return encrypted_memo;

            try
            {
                vector<char> decrypted = fc::aes_decrypt(encryption_key, m->encrypted);
                return fc::raw::unpack<std::string>(decrypted);
            }
            catch (...)
            {
            }
        }
    }
    return encrypted_memo;
}

map<uint32_t, applied_operation>
wallet_api::get_account_history(const std::string& account, uint32_t from, uint32_t limit)
{
    auto result = my->_remote_db->get_account_history(account, from, limit);
    if (!is_locked())
    {
        for (auto& item : result)
        {
            if (item.second.op.which() == operation::tag<transfer_operation>::value)
            {
                auto& top = item.second.op.get<transfer_operation>();
                top.memo = decrypt_memo(top.memo);
            }
        }
    }
    return result;
}

app::state wallet_api::get_state(const std::string& url)
{
    return my->_remote_db->get_state(url);
}

vector<withdraw_route> wallet_api::get_withdraw_routes(const std::string& account, withdraw_route_type type) const
{
    return my->_remote_db->get_withdraw_routes(account, type);
}

void wallet_api::set_transaction_expiration(uint32_t seconds)
{
    my->set_transaction_expiration(seconds);
}

annotated_signed_transaction
wallet_api::challenge(const std::string& challenger, const std::string& challenged, bool broadcast)
{
    // DEIP: TODO: remove whole method
    FC_ASSERT(false, "Challenge is disabled");
    /*
    FC_ASSERT( !is_locked() );

    challenge_authority_operation op;
    op.challenger = challenger;
    op.challenged = challenged;
    op.require_owner = false;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

    return my->sign_transaction( tx, broadcast );
    */
}

annotated_signed_transaction wallet_api::get_transaction(transaction_id_type id) const
{
    return my->_remote_db->get_transaction(id);
}

vector<grant_api_obj> wallet_api::list_my_grants()
{
    FC_ASSERT(!is_locked());

    try
    {
        my->use_remote_account_by_key_api();
    }
    catch (fc::exception& e)
    {
        elog("Connected node needs to enable account_by_key_api");
        return {};
    }

    vector<public_key_type> pub_keys;
    pub_keys.reserve(my->_keys.size());

    for (const auto& item : my->_keys)
        pub_keys.push_back(item.first);

    auto refs = (*my->_remote_account_by_key_api)->get_key_references(pub_keys);
    set<string> names;
    for (const auto& item : refs)
        for (const auto& name : item)
            names.insert(name);

    return my->_remote_db->get_grants(names);
}

set<string> wallet_api::list_grant_owners(const string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_grant_owners(lowerbound, limit);
}

vector<grant_api_obj> wallet_api::get_grants(const std::string& account_name)
{
    vector<grant_api_obj> result;

    result = my->_remote_db->get_grants({ account_name });

    return result;
}

annotated_signed_transaction wallet_api::create_grant(const std::string& grant_owner,
                                               const asset& balance,
                                               const uint32_t& start_block,
                                               const uint32_t& end_block,
                                               const discipline_name_type& target_discipline,
                                               const bool broadcast)
{
    FC_ASSERT(!is_locked());

    create_grant_operation op;

    op.owner = grant_owner;
    op.target_discipline = target_discipline;
    op.balance = balance;
    op.start_block = start_block;
    op.end_block = end_block;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::vote_for_review(const std::string& voter,
                                                         const int64_t review_id,
                                                         const int64_t discipline_id,
                                                         const int16_t weight,
                                                         const bool broadcast)
{
    FC_ASSERT(!is_locked());

    vote_for_review_operation op;

    op.voter = voter;
    op.review_id = review_id;
    op.discipline_id = discipline_id;
    op.weight = weight * DEIP_1_PERCENT;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::create_proposal(const std::string& creator,
                                                         const int64_t research_group_id,
                                                         const std::string& data,
                                                         const uint16_t action,
                                                         const time_point_sec expiration_time,
                                                         const bool broadcast)
{
    FC_ASSERT(!is_locked());

    create_proposal_operation op;

    op.creator = creator;
    op.research_group_id = research_group_id;
    op.data = data;
    op.action = action;
    op.expiration_time = expiration_time;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::make_review(const std::string& author,
                                                     const int64_t research_content_id,
                                                     const bool is_positive,
                                                     const std::string& content,
                                                     const bool broadcast)
{
    FC_ASSERT(!is_locked());

    make_review_operation op;

    op.author = author;
    op.research_content_id = research_content_id;
    op.is_positive = is_positive;
    op.content = content;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::contribute_to_token_sale(const int64_t research_token_sale_id,
                                                                  const std::string& owner,
                                                                  const uint32_t amount,
                                                                  const bool broadcast)
{
    FC_ASSERT(!is_locked());

    contribute_to_token_sale_operation op;

    op.research_token_sale_id = research_token_sale_id;
    op.owner = owner;
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::approve_research_group_invite(const int64_t research_group_invite_id,
                                                                       const std::string& owner,
                                                                       const bool broadcast)
{
    FC_ASSERT(!is_locked());

    approve_research_group_invite_operation op;

    op.research_group_invite_id = research_group_invite_id;
    op.owner = owner;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::reject_research_group_invite(const int64_t research_group_invite_id,
                                                                      const std::string &owner,
                                                                      const bool broadcast)
{
    FC_ASSERT(!is_locked());

    reject_research_group_invite_operation op;

    op.research_group_invite_id = research_group_invite_id;
    op.owner = owner;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::transfer_research_tokens_to_research_group(const int64_t research_token_id,
                                                                                    const int64_t research_id,
                                                                                    const std::string& owner,
                                                                                    const uint32_t amount,
                                                                                    const bool broadcast)
{
    FC_ASSERT(!is_locked());

    transfer_research_tokens_to_research_group_operation op;

    op.research_token_id = research_token_id;
    op.research_id = research_id;
    op.owner = owner;
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::research_update(const int64_t research_id,
                                                                   const std::string& title,
                                                                   const std::string& abstract,
                                                                   const std::string& permlink,
                                                                   const std::string& owner,
                                                                   const bool broadcast)
{
    FC_ASSERT(!is_locked());

    research_update_operation op;

    op.research_id = research_id;
    op.title = title;
    op.abstract = abstract;
    op.permlink = permlink;
    op.owner = owner;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::deposit_to_vesting_contract(const std::string& sender,
                                                                     const std::string& receiver,
                                                                     const uint32_t balance,
                                                                     const uint32_t withdrawal_period,
                                                                     const uint32_t contract_duration,
                                                                     const bool broadcast)
{
    FC_ASSERT(!is_locked());

    deposit_to_vesting_contract_operation op;

    op.sender = sender;
    op.receiver = receiver;
    op.balance = balance;
    op.withdrawal_period = withdrawal_period;
    op.contract_duration = contract_duration;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::withdraw_from_vesting_contract(const std::string& sender,
                                                                        const std::string& receiver,
                                                                        const uint32_t amount,
                                                                        const bool broadcast)
{
    FC_ASSERT(!is_locked());

    withdraw_from_vesting_contract_operation op;

    op.sender = sender;
    op.receiver = receiver;
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::vote_proposal(const std::string& voter,
                                                       const int64_t proposal_id,
                                                       const int64_t research_group_id,
                                                       const bool broadcast)
{
    FC_ASSERT(!is_locked());

    vote_proposal_operation op;

    op.voter = voter;
    op.proposal_id = proposal_id;
    op.research_group_id = research_group_id;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

namespace utils {

fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number)
{
    std::string sequence_string = std::to_string(sequence_number);
    fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
    fc::ecc::private_key derived_key = fc::ecc::private_key::regenerate(fc::sha256::hash(h));
    return derived_key;
}

brain_key_info suggest_brain_key()
{
    brain_key_info result;
    // create a private key for secure entropy
    fc::sha256 sha_entropy1 = fc::ecc::private_key::generate().get_secret();
    fc::sha256 sha_entropy2 = fc::ecc::private_key::generate().get_secret();
    fc::bigint entropy1(sha_entropy1.data(), sha_entropy1.data_size());
    fc::bigint entropy2(sha_entropy2.data(), sha_entropy2.data_size());
    fc::bigint entropy(entropy1);
    entropy <<= 8 * sha_entropy1.data_size();
    entropy += entropy2;
    string brain_key = "";

    for (int i = 0; i < BRAIN_KEY_WORD_COUNT; i++)
    {
        fc::bigint choice = entropy % graphene::words::word_list_size;
        entropy /= graphene::words::word_list_size;
        if (i > 0)
            brain_key += " ";
        brain_key += graphene::words::word_list[choice.to_int64()];
    }

    brain_key = detail::normalize_brain_key(brain_key);
    fc::ecc::private_key priv_key = derive_private_key(brain_key, 0);
    result.brain_priv_key = brain_key;
    result.wif_priv_key = key_to_wif(priv_key);
    result.pub_key = priv_key.get_public_key();
    return result;
}

} // namespace utils
} // namespace wallet
} // namespace deip
