#include <deip/protocol/get_config.hpp>
#include <deip/protocol/config.hpp>
#include <deip/protocol/asset.hpp>
#include <deip/protocol/types.hpp>
#include <deip/protocol/version.hpp>

namespace deip {
namespace protocol {

fc::variant_object get_config()
{
    fc::mutable_variant_object result;

    // clang-format off

#ifdef IS_TEST_NET
    result[ "IS_TEST_NET" ] = true;
#else
    result[ "IS_TEST_NET" ] = false;
#endif

    // clang-format on

    result["DEIP_100_PERCENT"] = DEIP_100_PERCENT;
    result["DEIP_1_PERCENT"] = DEIP_1_PERCENT;
    result["DEIP_1_TENTH_PERCENT"] = DEIP_1_TENTH_PERCENT;
    result["DEIP_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD"] = DEIP_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
    result["DEIP_ADDRESS_PREFIX"] = DEIP_ADDRESS_PREFIX;
    result["DEIP_APR_PERCENT_MULTIPLY_PER_BLOCK"] = DEIP_APR_PERCENT_MULTIPLY_PER_BLOCK;
    result["DEIP_APR_PERCENT_MULTIPLY_PER_HOUR"] = DEIP_APR_PERCENT_MULTIPLY_PER_HOUR;
    result["DEIP_APR_PERCENT_MULTIPLY_PER_ROUND"] = DEIP_APR_PERCENT_MULTIPLY_PER_ROUND;
    result["DEIP_APR_PERCENT_SHIFT_PER_BLOCK"] = DEIP_APR_PERCENT_SHIFT_PER_BLOCK;
    result["DEIP_APR_PERCENT_SHIFT_PER_HOUR"] = DEIP_APR_PERCENT_SHIFT_PER_HOUR;
    result["DEIP_APR_PERCENT_SHIFT_PER_ROUND"] = DEIP_APR_PERCENT_SHIFT_PER_ROUND;
    result["DEIP_BANDWIDTH_AVERAGE_WINDOW_SECONDS"] = DEIP_BANDWIDTH_AVERAGE_WINDOW_SECONDS;
    result["DEIP_BANDWIDTH_PRECISION"] = DEIP_BANDWIDTH_PRECISION;
    result["DEIP_BLOCKCHAIN_HARDFORK_VERSION"] = DEIP_BLOCKCHAIN_HARDFORK_VERSION;
    result["DEIP_BLOCKCHAIN_VERSION"] = DEIP_BLOCKCHAIN_VERSION;
    result["DEIP_BLOCK_INTERVAL"] = DEIP_BLOCK_INTERVAL;
    result["DEIP_BLOCKS_PER_DAY"] = DEIP_BLOCKS_PER_DAY;
    result["DEIP_BLOCKS_PER_HOUR"] = DEIP_BLOCKS_PER_HOUR;
    result["DEIP_BLOCKS_PER_YEAR"] = DEIP_BLOCKS_PER_YEAR;
    result["DEIP_CASHOUT_WINDOW_SECONDS"] = DEIP_CASHOUT_WINDOW_SECONDS;
    result["DEIP_CONTENT_APR_PERCENT"] = DEIP_CONTENT_APR_PERCENT;
    result["DEIP_CREATE_ACCOUNT_DELEGATION_RATIO"] = DEIP_CREATE_ACCOUNT_DELEGATION_RATIO;
    result["DEIP_CREATE_ACCOUNT_DELEGATION_TIME"] = DEIP_CREATE_ACCOUNT_DELEGATION_TIME;
    result["DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER"] = DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER;
    result["DEIP_HARDFORK_REQUIRED_WITNESSES"] = DEIP_HARDFORK_REQUIRED_WITNESSES;
    result["DEIP_INFLATION_NARROWING_PERIOD"] = DEIP_INFLATION_NARROWING_PERIOD;
    result["DEIP_INFLATION_RATE_START_PERCENT"] = DEIP_INFLATION_RATE_START_PERCENT;
    result["DEIP_INFLATION_RATE_STOP_PERCENT"] = DEIP_INFLATION_RATE_STOP_PERCENT;
    result["DEIP_IRREVERSIBLE_THRESHOLD"] = DEIP_IRREVERSIBLE_THRESHOLD;
    result["DEIP_MAX_ACCOUNT_NAME_LENGTH"] = DEIP_MAX_ACCOUNT_NAME_LENGTH;
    result["DEIP_MAX_ACCOUNT_WITNESS_VOTES"] = DEIP_MAX_ACCOUNT_WITNESS_VOTES;
    result["DEIP_MAX_BLOCK_SIZE"] = DEIP_MAX_BLOCK_SIZE;
    result["DEIP_MAX_FEED_AGE_SECONDS"] = DEIP_MAX_FEED_AGE_SECONDS;
    result["DEIP_MAX_MEMO_SIZE"] = DEIP_MAX_MEMO_SIZE;
    result["DEIP_MAX_WITNESSES"] = DEIP_MAX_WITNESSES;
    result["DEIP_MAX_PERMLINK_LENGTH"] = DEIP_MAX_PERMLINK_LENGTH;
    result["DEIP_MAX_PROXY_RECURSION_DEPTH"] = DEIP_MAX_PROXY_RECURSION_DEPTH;
    result["DEIP_MAX_RESERVE_RATIO"] = DEIP_MAX_RESERVE_RATIO;
    result["DEIP_MAX_RUNNER_WITNESSES"] = DEIP_MAX_RUNNER_WITNESSES;
    result["DEIP_MAX_SHARE_SUPPLY"] = DEIP_MAX_SHARE_SUPPLY;
    result["DEIP_MAX_SIG_CHECK_DEPTH"] = DEIP_MAX_SIG_CHECK_DEPTH;
    result["DEIP_MAX_TIME_UNTIL_EXPIRATION"] = DEIP_MAX_TIME_UNTIL_EXPIRATION;
    result["DEIP_MAX_TRANSACTION_SIZE"] = DEIP_MAX_TRANSACTION_SIZE;
    result["DEIP_MAX_UNDO_HISTORY"] = DEIP_MAX_UNDO_HISTORY;
    result["DEIP_MAX_VOTE_CHANGES"] = DEIP_MAX_VOTE_CHANGES;
    result["DEIP_MAX_VOTED_WITNESSES"] = DEIP_MAX_VOTED_WITNESSES;
    result["DEIP_MAX_WITHDRAW_ROUTES"] = DEIP_MAX_WITHDRAW_ROUTES;
    result["DEIP_MAX_WITNESS_URL_LENGTH"] = DEIP_MAX_WITNESS_URL_LENGTH;
    result["DEIP_MIN_ACCOUNT_CREATION_FEE"] = DEIP_MIN_ACCOUNT_CREATION_FEE;
    result["DEIP_MIN_ACCOUNT_NAME_LENGTH"] = DEIP_MIN_ACCOUNT_NAME_LENGTH;
    result["DEIP_MIN_BLOCK_SIZE_LIMIT"] = DEIP_MIN_BLOCK_SIZE_LIMIT;
    result["DEIP_MIN_PERMLINK_LENGTH"] = DEIP_MIN_PERMLINK_LENGTH;
    result["DEIP_MIN_REPLY_INTERVAL"] = DEIP_MIN_REPLY_INTERVAL;
    result["DEIP_MIN_VOTE_INTERVAL_SEC"] = DEIP_MIN_VOTE_INTERVAL_SEC;
    result["DEIP_MIN_FEEDS"] = DEIP_MIN_FEEDS;
    result["DEIP_MINING_REWARD"] = DEIP_MINING_REWARD;
    result["DEIP_MIN_PAYOUT"] = DEIP_MIN_PAYOUT;
    result["DEIP_MIN_POW_REWARD"] = DEIP_MIN_POW_REWARD;
    result["DEIP_MIN_PRODUCER_REWARD"] = DEIP_MIN_PRODUCER_REWARD;
    result["DEIP_MIN_TRANSACTION_EXPIRATION_LIMIT"] = DEIP_MIN_TRANSACTION_EXPIRATION_LIMIT;
    result["DEIP_MIN_UNDO_HISTORY"] = DEIP_MIN_UNDO_HISTORY;
    result["DEIP_NUM_INIT_DELEGATES"] = DEIP_NUM_INIT_DELEGATES;
    result["DEIP_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM"] = DEIP_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM;
    result["DEIP_OWNER_AUTH_RECOVERY_PERIOD"] = DEIP_OWNER_AUTH_RECOVERY_PERIOD;
    result["DEIP_OWNER_UPDATE_LIMIT"] = DEIP_OWNER_UPDATE_LIMIT;
    result["DEIP_POST_REWARD_FUND_NAME"] = DEIP_POST_REWARD_FUND_NAME;
    result["DEIP_POW_APR_PERCENT"] = DEIP_POW_APR_PERCENT;
    result["DEIP_PRODUCER_APR_PERCENT"] = DEIP_PRODUCER_APR_PERCENT;
    result["DEIP_PROXY_TO_SELF_ACCOUNT"] = DEIP_PROXY_TO_SELF_ACCOUNT;
    result["DEIP_RECENT_RSHARES_DECAY_RATE"] = DEIP_RECENT_RSHARES_DECAY_RATE;
    result["DEIP_REVERSE_AUCTION_WINDOW_SECONDS"] = DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
    result["DEIP_ROOT_POST_PARENT"] = DEIP_ROOT_POST_PARENT;
    result["DEIP_SAVINGS_WITHDRAW_REQUEST_LIMIT"] = DEIP_SAVINGS_WITHDRAW_REQUEST_LIMIT;
    result["DEIP_SAVINGS_WITHDRAW_TIME"] = DEIP_SAVINGS_WITHDRAW_TIME;
    result["DEIP_START_MINER_VOTING_BLOCK"] = DEIP_START_MINER_VOTING_BLOCK;
    result["DEIP_START_VESTING_BLOCK"] = DEIP_START_VESTING_BLOCK;
    result["DEIP_UPVOTE_LOCKOUT"] = DEIP_UPVOTE_LOCKOUT;
    result["DEIP_VESTING_FUND_PERCENT"] = DEIP_VESTING_FUND_PERCENT;
    result["DEIP_VESTING_WITHDRAW_INTERVALS"] = DEIP_VESTING_WITHDRAW_INTERVALS;
    result["DEIP_VESTING_WITHDRAW_INTERVALS_PRE_HF_16"] = DEIP_VESTING_WITHDRAW_INTERVALS_PRE_HF_16;
    result["DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS"] = DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS;
    result["DEIP_VOTE_DUST_THRESHOLD"] = DEIP_VOTE_DUST_THRESHOLD;
    result["DEIP_VOTE_REGENERATION_SECONDS"] = DEIP_VOTE_REGENERATION_SECONDS;
    result["DEIP_SYMBOL"] = DEIP_SYMBOL;
    result["VESTS_SYMBOL"] = VESTS_SYMBOL;
    result["VIRTUAL_SCHEDULE_LAP_LENGTH"] = VIRTUAL_SCHEDULE_LAP_LENGTH;
    result["DEIP_REWARDS_INITIAL_SUPPLY"] = DEIP_REWARDS_INITIAL_SUPPLY;
    result["DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS"] = DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS;
    result["DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS"] = DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS;
    result["DEIP_REWARD_INCREASE_THRESHOLD_IN_DAYS"] = DEIP_REWARD_INCREASE_THRESHOLD_IN_DAYS;
    result["DEIP_ADJUST_REWARD_PERCENT"] = DEIP_ADJUST_REWARD_PERCENT;
    result["DEIP_LIMIT_BUDGETS_PER_OWNER"] = DEIP_LIMIT_BUDGETS_PER_OWNER;
    result["DEIP_LIMIT_BUDGETS_LIST_SIZE"] = DEIP_LIMIT_BUDGETS_LIST_SIZE;
    result["DEIP_LIMIT_API_BUDGETS_LIST_SIZE"] = DEIP_LIMIT_API_BUDGETS_LIST_SIZE;

    return result;
}
} // namespace protocol
} // namespace deip
