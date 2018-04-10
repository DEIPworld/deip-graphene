#pragma once

#include <deip/app/application.hpp>
#include <deip/chain/database.hpp>
#include <deip/chain/genesis_state.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_research.hpp>
#include <deip/chain/dbs_discipline.hpp>
#include <deip/chain/dbs_research.hpp>
#include <deip/chain/dbs_research_content.hpp>
#include <deip/chain/dbs_vote.hpp>
#include <deip/chain/dbs_expert_token.hpp>
#include <deip/chain/dbs_research_group_invite.hpp>
#include <deip/chain/dbs_research_group_join_request.hpp>
#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <deip/plugins/debug_node/debug_node_plugin.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <iostream>

#include "defines.hpp"

using namespace graphene::db;

namespace deip {
namespace chain {

using namespace deip::protocol;

void create_initdelegate_for_genesis_state(genesis_state_type& genesis_state);
void create_initdelegate_expert_tokens_for_genesis_state(genesis_state_type& genesis_state);
void create_disciplines_for_genesis_state(genesis_state_type& genesis_state);

struct database_fixture
{
    // the reason we use an app is to exercise the indexes of built-in
    //   plugins
    deip::app::application app;
    chain::database& db;
    genesis_state_type genesis_state;
    signed_transaction trx;
    public_key_type committee_key;
    account_id_type committee_account;

    const private_key_type init_account_priv_key;
    const public_key_type init_account_pub_key;

    const std::string debug_key;
    const uint32_t default_skip;

    std::shared_ptr<deip::plugin::debug_node::debug_node_plugin> db_plugin;

    optional<fc::temp_directory> data_dir;

    database_fixture();
    ~database_fixture();

    static private_key_type generate_private_key(const std::string& seed);
    void open_database();

    void generate_block(uint32_t skip = 0,
                        const private_key_type& key = generate_private_key("init_key"),
                        int miss_blocks = 0);

    /**
     * @brief Generates block_count blocks
     * @param block_count number of blocks to generate
     */
    void generate_blocks(uint32_t block_count);

    /**
     * @brief Generates blocks until the head block time matches or exceeds timestamp
     * @param timestamp target time to generate blocks until
     */
    void generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks = true);

    const account_object& account_create(const string& name,
                                         const string& creator,
                                         const private_key_type& creator_key,
                                         const share_type& fee,
                                         const public_key_type& key,
                                         const public_key_type& post_key,
                                         const string& json_metadata);

    const account_object&
    account_create(const string& name, const public_key_type& key, const public_key_type& post_key);

    const account_object& account_create(const string& name, const public_key_type& key);

    const witness_object& witness_create(const string& owner,
                                         const private_key_type& owner_key,
                                         const string& url,
                                         const public_key_type& signing_key,
                                         const share_type& fee);

    const discipline_object& discipline_create(const discipline_id_type& id, 
                                                const discipline_name_type& name, 
                                                const discipline_id_type& parent_id,
                                                const share_type& votes_in_last_ten_weeks);

    const research_group_object& research_group_create(const int64_t& id,
                                                       const string& name, 
                                                       const string& permlink,
                                                       const string& desciption,
                                                       const share_type funds,
                                                       const share_type quorum_percent);
                                                       
    const research_group_object& research_group_create_by_operation(const account_name_type& creator,
                                                                    const string& name,
                                                                    const string& permlink,
                                                                    const string& description,
                                                                    const uint32_t quorum_percent);

    const research_group_token_object& research_group_token_create(const research_group_id_type& research_group_id,
                                                                   const account_name_type& account,
                                                                   const share_type amount);

    const research_group_object& setup_research_group(const int64_t &id,
                                                      const string &name,
                                                      const string &permlink,
                                                      const string &desciption,
                                                      const share_type funds,
                                                      const share_type quorum_percent,
                                                      const vector<std::pair<account_name_type, share_type>> &accounts);

    const proposal_object& create_proposal(const int64_t id,
                                           const dbs_proposal::action_t action,
                                           const string json_data,
                                           const account_name_type& creator,
                                           const research_group_id_type& research_group_id,
                                           const time_point_sec expiration_time,
                                           const share_type quorum_percent);

    void create_proposal_by_operation(const account_name_type& creator,
                                      const research_group_id_type& research_group_id,
                                      const std::string json_data,
                                      const dbs_proposal::action_t action,
                                      const fc::time_point_sec expiration_time);

    void create_disciplines();

    const research_object& research_create(const int64_t id,
                                           const string& title,
                                           const string& abstract,
                                           const string& permlink,
                                           const research_group_id_type& research_group_id,
                                           const uint16_t review_share_in_percent,
                                           const uint16_t dropout_compensation_in_percent);


    const research_content_object& research_content_create(
                                            const int64_t& id,
                                            const int64_t& research_id,
                                            const research_content_type& type,
                                            const std::string& title,
                                            const std::string& content,
                                            const int16_t& activity_round,
                                            const research_content_activity_state& activity_state,
                                            const time_point_sec& activity_window_start,
                                            const time_point_sec& activity_window_end,
                                            const std::vector<account_name_type>& authors,
                                            const std::vector<research_content_id_type>& references,
                                            const std::vector<string>& external_references);


    const expert_token_object& expert_token_create(const int64_t id,
                                                   const account_name_type& account,
                                                   const discipline_id_type& discipline_id,
                                                   const share_type& amount);

    const research_group_invite_object& research_group_invite_create(const int64_t id,
                                                                     const account_name_type& account_name,
                                                                     const research_group_id_type& research_group_id,
                                                                     const share_type research_group_token_amount);

    const research_group_join_request_object& research_group_join_request_create(const uint32_t id,
                                                                                 const account_name_type& account_name,
                                                                                 const research_group_id_type& research_group_id,
                                                                                 const std::string motivation_letter);

    void fund(const string& account_name, const share_type& amount = 500000);
    void fund(const string& account_name, const asset& amount);
    void transfer(const string& from, const string& to, const share_type& deip);
    void convert(const string& account_name, const asset& amount);
    void vest(const string& from, const share_type& amount);
    void vest(const string& account, const asset& amount);
    void create_all_discipline_expert_tokens_for_account(const string& account);
    const expert_token_object& common_token(const string& account, const share_type& amount);
    const expert_token_object& expert_token(const string& account, const discipline_id_type& discipline_id, const share_type& amount);
    void proxy(const string& account, const string& proxy);
    const asset& get_balance(const string& account_name) const;
    void sign(signed_transaction& trx, const fc::ecc::private_key& key);

    vector<operation> get_last_operations(uint32_t ops);

    void validate_database(void);
};

struct clean_database_fixture : public database_fixture
{
    clean_database_fixture();
    ~clean_database_fixture();

    void resize_shared_mem(uint64_t size);
};

struct live_database_fixture : public database_fixture
{
    live_database_fixture();
    ~live_database_fixture();

    fc::path _chain_dir;
};

struct timed_blocks_database_fixture : public clean_database_fixture
{
    timed_blocks_database_fixture();

    fc::time_point_sec default_deadline;
    const int BLOCK_LIMIT_DEFAULT = 5;

private:
    static bool _time_printed;
};

namespace test {
bool _push_block(database& db, const signed_block& b, uint32_t skip_flags = 0);
void _push_transaction(database& db, const signed_transaction& tx, uint32_t skip_flags = 0);
} // namespace test
} // namespace chain
} // namespace deip
