/*
 * Copyright (c) 2016 Steemit, Inc., and contributors.
 */

// clang-format off

#pragma once

#define DEIP_BLOCKCHAIN_VERSION              ( version(0, 0, 1) )
#define DEIP_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( DEIP_BLOCKCHAIN_VERSION ) )

#define DEIP_ADDRESS_PREFIX                  "DEIP"

#ifdef IS_TEST_NET
#define VESTS_SYMBOL   (uint64_t(6) | (uint64_t('V') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< VESTS with 6 digits of precision
#define DEIP_SYMBOL  (uint64_t(3) | (uint64_t('T') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< DEIP with 3 digits of precision

#define DEIP_CASHOUT_WINDOW_SECONDS          (60*60) /// 1 hr
#define DEIP_UPVOTE_LOCKOUT                  (fc::minutes(5))

#define DEIP_MIN_ACCOUNT_CREATION_FEE        0

#define DEIP_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define DEIP_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define DEIP_OWNER_UPDATE_LIMIT                          fc::seconds(0)
#define DEIP_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 1

#define DEIP_REWARDS_INITIAL_SUPPLY                   asset( 1000000, DEIP_SYMBOL )
#define DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS    5

#define DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS   2
#define DEIP_REWARD_INCREASE_THRESHOLD_IN_DAYS        3
#define DEIP_ADJUST_REWARD_PERCENT                    5

#define DEIP_LIMIT_BUDGETS_PER_OWNER          5
#define DEIP_LIMIT_BUDGETS_LIST_SIZE          DEIP_LIMIT_BUDGETS_PER_OWNER
#define DEIP_LIMIT_API_BUDGETS_LIST_SIZE      DEIP_LIMIT_BUDGETS_PER_OWNER

#else // IS LIVE DEIP NETWORK
#define VESTS_SYMBOL  (uint64_t(6) | (uint64_t('V') << 8) | (uint64_t('E') << 16) | (uint64_t('S') << 24) | (uint64_t('T') << 32) | (uint64_t('S') << 40)) ///< VESTS with 6 digits of precision
#define DEIP_SYMBOL (uint64_t(3) | (uint64_t('D') << 8) | (uint64_t('E') << 16) | (uint64_t('I') << 24) | (uint64_t('P') << 32)) ///< DEIP with 3 digits of precision

#define DEIP_CASHOUT_WINDOW_SECONDS          (60*60*24*7)  /// 7 days
#define DEIP_UPVOTE_LOCKOUT                  (fc::hours(12))

#define DEIP_MIN_ACCOUNT_CREATION_FEE           1

#define DEIP_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define DEIP_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define DEIP_OWNER_UPDATE_LIMIT                          fc::minutes(60)
#define DEIP_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 3186477

#define DEIP_REWARDS_INITIAL_SUPPLY                   asset( 2300000000, DEIP_SYMBOL )
#define DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS    (2 * 365)

#define DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS   30
#define DEIP_REWARD_INCREASE_THRESHOLD_IN_DAYS        100
#define DEIP_ADJUST_REWARD_PERCENT                    5

#define DEIP_LIMIT_BUDGETS_PER_OWNER          1000
#define DEIP_LIMIT_BUDGETS_LIST_SIZE          1000
#define DEIP_LIMIT_API_BUDGETS_LIST_SIZE      1000

#endif

#define DEIP_BLOCK_INTERVAL                  3
#define DEIP_BLOCKS_PER_YEAR                 (365*24*60*60/DEIP_BLOCK_INTERVAL)
#define DEIP_BLOCKS_PER_DAY                  (24*60*60/DEIP_BLOCK_INTERVAL)
#define DEIP_BLOCKS_PER_HOUR                 (60*60/DEIP_BLOCK_INTERVAL)
#define DEIP_START_VESTING_BLOCK             (DEIP_BLOCKS_PER_DAY * 7)
#define DEIP_START_MINER_VOTING_BLOCK        (DEIP_BLOCKS_PER_DAY * 30)

#define DEIP_NUM_INIT_DELEGATES              1

#define DEIP_MAX_VOTED_WITNESSES              20
#define DEIP_MAX_RUNNER_WITNESSES             1
#define DEIP_MAX_WITNESSES                    (DEIP_MAX_VOTED_WITNESSES+DEIP_MAX_RUNNER_WITNESSES)
#define DEIP_WITNESS_MISSED_BLOCKS_THRESHOLD  DEIP_BLOCKS_PER_DAY/2
#define DEIP_HARDFORK_REQUIRED_WITNESSES      17 // 17 of the 21 dpos witnesses (20 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.

#define DEIP_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define DEIP_MAX_MEMO_SIZE                   2048
#define DEIP_MAX_PROXY_RECURSION_DEPTH       4
#define DEIP_VESTING_WITHDRAW_INTERVALS_PRE_HF_16 104
#define DEIP_VESTING_WITHDRAW_INTERVALS      13
#define DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS (60*60*24*7) /// 1 week per interval
#define DEIP_MAX_WITHDRAW_ROUTES             10
#define DEIP_SAVINGS_WITHDRAW_TIME           (fc::days(3))
#define DEIP_SAVINGS_WITHDRAW_REQUEST_LIMIT  100
#define DEIP_VOTE_REGENERATION_SECONDS       (5*60*60*24) // 5 day
#define DEIP_MAX_VOTE_CHANGES                5
#define DEIP_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) /// 30 minutes
#define DEIP_MIN_VOTE_INTERVAL_SEC           3
#define DEIP_VOTE_DUST_THRESHOLD             (50000000)

#define DEIP_MIN_ROOT_COMMENT_INTERVAL       (fc::seconds(60*5)) // 5 minutes
#define DEIP_MIN_REPLY_INTERVAL              (fc::seconds(20)) // 20 seconds

#define DEIP_MAX_ACCOUNT_WITNESS_VOTES       30

#define DEIP_100_PERCENT                     10000
#define DEIP_1_PERCENT                       (DEIP_100_PERCENT/100)
#define DEIP_1_TENTH_PERCENT                 (DEIP_100_PERCENT/1000)

#define DEIP_INFLATION_RATE_START_PERCENT    (978) // Fixes block 7,000,000 to 9.5%
#define DEIP_INFLATION_RATE_STOP_PERCENT     (95) // 0.95%
#define DEIP_INFLATION_NARROWING_PERIOD      (250000) // Narrow 0.01% every 250k blocks
#define DEIP_CONTENT_REWARD_PERCENT          (75*DEIP_1_PERCENT) //75% of inflation, 7.125% inflation
#define DEIP_VESTING_FUND_PERCENT            (15*DEIP_1_PERCENT) //15% of inflation, 1.425% inflation

#define DEIP_BANDWIDTH_AVERAGE_WINDOW_SECONDS (60*60*24*7) ///< 1 week
#define DEIP_BANDWIDTH_PRECISION              (uint64_t(1000000)) ///< 1 million
#define DEIP_MAX_COMMENT_DEPTH                0xffff // 64k
#define DEIP_SOFT_MAX_COMMENT_DEPTH           0xff // 255

#define DEIP_MAX_RESERVE_RATIO                (20000)

#define DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER 30
#define DEIP_CREATE_ACCOUNT_DELEGATION_RATIO     5
#define DEIP_CREATE_ACCOUNT_DELEGATION_TIME      fc::days(30)

#define DEIP_MINING_REWARD                   asset( 1000, DEIP_SYMBOL )

#define DEIP_MIN_CONTENT_REWARD              DEIP_MINING_REWARD
#define DEIP_MIN_CURATE_REWARD               DEIP_MINING_REWARD
#define DEIP_MIN_PRODUCER_REWARD             DEIP_MINING_REWARD
#define DEIP_MIN_POW_REWARD                  DEIP_MINING_REWARD

#define DEIP_POST_REWARD_FUND_NAME           ("post")
#define DEIP_COMMENT_REWARD_FUND_NAME        ("comment")
#define DEIP_RECENT_RSHARES_DECAY_RATE       (fc::days(15))
// note, if redefining these constants make sure calculate_claims doesn't overflow

// 5ccc e802 de5f
// int(expm1( log1p( 1 ) / BLOCKS_PER_YEAR ) * 2**DEIP_APR_PERCENT_SHIFT_PER_BLOCK / 100000 + 0.5)
// we use 100000 here instead of 10000 because we end up creating an additional 9x for vesting
#define DEIP_APR_PERCENT_MULTIPLY_PER_BLOCK          ( (uint64_t( 0x5ccc ) << 0x20) \
                                                        | (uint64_t( 0xe802 ) << 0x10) \
                                                        | (uint64_t( 0xde5f )        ) \
                                                        )
// chosen to be the maximal value such that DEIP_APR_PERCENT_MULTIPLY_PER_BLOCK * 2**64 * 100000 < 2**128
#define DEIP_APR_PERCENT_SHIFT_PER_BLOCK             87

#define DEIP_APR_PERCENT_MULTIPLY_PER_ROUND          ( (uint64_t( 0x79cc ) << 0x20 ) \
                                                        | (uint64_t( 0xf5c7 ) << 0x10 ) \
                                                        | (uint64_t( 0x3480 )         ) \
                                                        )

#define DEIP_APR_PERCENT_SHIFT_PER_ROUND             83

// We have different constants for hourly rewards
// i.e. hex(int(math.expm1( math.log1p( 1 ) / HOURS_PER_YEAR ) * 2**DEIP_APR_PERCENT_SHIFT_PER_HOUR / 100000 + 0.5))
#define DEIP_APR_PERCENT_MULTIPLY_PER_HOUR           ( (uint64_t( 0x6cc1 ) << 0x20) \
                                                        | (uint64_t( 0x39a1 ) << 0x10) \
                                                        | (uint64_t( 0x5cbd )        ) \
                                                        )

// chosen to be the maximal value such that DEIP_APR_PERCENT_MULTIPLY_PER_HOUR * 2**64 * 100000 < 2**128
#define DEIP_APR_PERCENT_SHIFT_PER_HOUR              77

// These constants add up to GRAPHENE_100_PERCENT.  Each GRAPHENE_1_PERCENT is equivalent to 1% per year APY
// *including the corresponding 9x vesting rewards*
#define DEIP_CONTENT_APR_PERCENT             3875
#define DEIP_PRODUCER_APR_PERCENT             750
#define DEIP_POW_APR_PERCENT                  750

#define DEIP_MIN_PAYOUT                  (asset(5,DEIP_SYMBOL))

#define DEIP_MIN_ACCOUNT_NAME_LENGTH          3
#define DEIP_MAX_ACCOUNT_NAME_LENGTH         16

#define DEIP_MIN_PERMLINK_LENGTH             0
#define DEIP_MAX_PERMLINK_LENGTH             256
#define DEIP_MAX_WITNESS_URL_LENGTH          2048

#define DEIP_MAX_SHARE_SUPPLY                int64_t(1000000000000000ll)
#define DEIP_MAX_SIG_CHECK_DEPTH             2

#define DEIP_MAX_TRANSACTION_SIZE            (1024*64)
#define DEIP_MIN_BLOCK_SIZE_LIMIT            (DEIP_MAX_TRANSACTION_SIZE)
#define DEIP_MAX_BLOCK_SIZE                  (DEIP_MAX_TRANSACTION_SIZE*DEIP_BLOCK_INTERVAL*2000)
#define DEIP_MAX_FEED_AGE_SECONDS            (60*60*24*7) // 7 days
#define DEIP_MIN_FEEDS                       (DEIP_MAX_WITNESSES/3) /// protects the network from conversions before price has been established

#define DEIP_MIN_UNDO_HISTORY                10
#define DEIP_MAX_UNDO_HISTORY                10000

#define DEIP_MIN_TRANSACTION_EXPIRATION_LIMIT (DEIP_BLOCK_INTERVAL * 5) // 5 transactions per block

#define DEIP_IRREVERSIBLE_THRESHOLD          (75 * DEIP_1_PERCENT)

#define VIRTUAL_SCHEDULE_LAP_LENGTH ( fc::uint128::max_value() )

/**
 *  Reserved Account IDs with special meaning
 */
///@{

/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define DEIP_PROXY_TO_SELF_ACCOUNT           ""
/// Represents the canonical root post parent account
#define DEIP_ROOT_POST_PARENT                (account_name_type())

///@}

// clang-format on