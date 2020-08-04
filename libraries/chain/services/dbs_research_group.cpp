#include <deip/chain/util/permlink.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_research_group::dbs_research_group(database& db)
  : _base_type(db)
{
}

const research_group_object& dbs_research_group::get_research_group(const research_group_id_type& id) const 
{
  try { return db_impl().get<research_group_object, by_id>(id); }
  FC_CAPTURE_AND_RETHROW((id))
}

const dbs_research_group::research_group_optional_ref_type dbs_research_group::get_research_group_if_exists(const research_group_id_type& id) const
{
    research_group_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

/* [DEPRECATED] */
const research_group_object& dbs_research_group::get_research_group_by_permlink(const fc::string& permlink) const 
{
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_permlink>();

    auto itr = idx.find(permlink, fc::strcmp_less());
    FC_ASSERT(itr != idx.end(), "Research group with permlink \"${1}\" does not exist.", ("1", permlink));
    return *itr;
}

const dbs_research_group::research_group_optional_ref_type dbs_research_group::get_research_group_by_permlink_if_exists(const string& permlink) const
{
    research_group_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_permlink>();

    auto itr = idx.find(permlink, fc::strcmp_less());
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const research_group_object& dbs_research_group::get_research_group_by_account(const account_name_type& account) const
{
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_account>();

    auto itr = idx.find(account);
    FC_ASSERT(itr != idx.end(), "Research group account \"${1}\" does not exist.", ("1", account));
    return *itr;
}

const dbs_research_group::research_group_optional_ref_type dbs_research_group::get_research_group_by_account_if_exists(const account_name_type& account) const
{
    research_group_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_account>();

    auto itr = idx.find(account);
    if (itr != idx.end())
    {
        result = *itr;
    }
    return result;
}

const research_group_object& dbs_research_group::create_personal_research_group(const account_name_type& account)
{
    const research_group_object& personal_research_group
        = db_impl().create<research_group_object>([&](research_group_object& rg_o) {
              rg_o.account = account;
              rg_o.creator = account;
              fc::from_string(rg_o.name, account);
              fc::from_string(rg_o.permlink, account);
              fc::from_string(rg_o.description, account);
              rg_o.management_model_v = 2;
              rg_o.is_personal = true;
              rg_o.is_centralized = false;
              rg_o.is_dao = false;
              rg_o.is_created_by_organization = false;
              rg_o.has_organization = false;
          });

    return personal_research_group;
}

const research_group_object& dbs_research_group::create_research_group(
  const account_name_type& account,
  const account_name_type& creator,
  const std::string& name,
  const string& description)
{
    const auto& research_group_permlink = deip::chain::util::generate_permlink(name);
    const research_group_object& research_group
        = db_impl().create<research_group_object>([&](research_group_object& rg_o) {
              rg_o.account = account;
              rg_o.creator = creator;
              fc::from_string(rg_o.name, name);
              fc::from_string(rg_o.description, description);
              fc::from_string(rg_o.permlink, research_group_permlink);
              rg_o.is_personal = false;
              rg_o.is_centralized = false;
              rg_o.is_dao = true;
              rg_o.default_quorum = DEIP_100_PERCENT;
          });

    return research_group;
}

const research_group_object& dbs_research_group::update_research_group(
  const research_group_object& research_group,
  const string& name,
  const string& description) 
{
    const auto& research_group_permlink = deip::chain::util::generate_permlink(name);
    db_impl().modify(research_group, [&](research_group_object& rg_o) {
        fc::from_string(rg_o.name, name);
        fc::from_string(rg_o.description, description);
        fc::from_string(rg_o.permlink, research_group_permlink);
    });

    return research_group;
}

void dbs_research_group::check_research_group_existence(const research_group_id_type& research_group_id) const
{
  const auto& idx = db_impl().get_index<research_group_index>().indices().get<by_id>();
  FC_ASSERT(idx.find(research_group_id) != idx.cend(), "Group \"${1}\" does not exist.", ("1", research_group_id));
}

const bool dbs_research_group::research_group_exists(const research_group_id_type& research_group_id) const
{
  const auto& idx = db_impl()
    .get_index<research_group_index>()
    .indices()
    .get<by_id>();

  return idx.find(research_group_id) != idx.end();
}

/* [DEPRECATED] */
const bool dbs_research_group::research_group_exists(const string& permlink) const
{
  const auto& idx = db_impl()
    .get_index<research_group_index>()
    .indices()
    .get<by_permlink>();

  return idx.find(permlink, fc::strcmp_less()) != idx.end();
}

const bool dbs_research_group::research_group_exists(const account_name_type& account) const
{
    const auto& idx = db_impl()
            .get_index<research_group_index>()
            .indices()
            .get<by_account>();

    return idx.find(account) != idx.end();
}

const research_group_token_object& dbs_research_group::get_research_group_token_by_id(const research_group_token_id_type& id) const 
{
  try { return db_impl().get<research_group_token_object>(id); }
  FC_CAPTURE_AND_RETHROW((id))
}

dbs_research_group::research_group_token_refs_type dbs_research_group::get_research_group_tokens_by_member(const account_name_type& member) const
{
  research_group_token_refs_type ret;

  const auto& idx = db_impl()
    .get_index<research_group_token_index>()
    .indicies()
    .get<by_owner>();

   auto it_pair = idx.equal_range(member);

   auto it = it_pair.first;
   const auto it_end = it_pair.second;
   while (it != it_end)
   {
       ret.push_back(std::cref(*it));
       ++it;
  }

  return ret;
}

dbs_research_group::research_group_token_refs_type dbs_research_group::get_research_group_tokens(const research_group_id_type& internal_id) const
{
  research_group_token_refs_type ret;
  const auto& idx = db_impl()
    .get_index<research_group_token_index>()
    .indicies()
    .get<by_research_group>();

  auto it_pair = idx.equal_range(internal_id);

  auto it = it_pair.first;
  const auto it_end = it_pair.second;
  while (it != it_end)
  {
    ret.push_back(std::cref(*it));
    ++it;
  }

  return ret;
}

const research_group_token_object& dbs_research_group::get_research_group_token_by_member(
  const account_name_type& member,
  const research_group_id_type& internal_id) const
{
  const auto& idx = db_impl()
    .get_index<research_group_token_index>()
    .indicies()
    .get<by_owner>();

  auto itr = idx.find(std::make_tuple(member, internal_id));
  FC_ASSERT(itr != idx.end(), "Research group ${1} token for ${2} does not exist", ("1", internal_id)("2", member));
  return *itr;
}


const dbs_research_group::research_group_token_optional_ref_type dbs_research_group::get_research_group_token_by_member_if_exists(
  const account_name_type& member,
  const research_group_id_type& internal_id) const
{
    research_group_token_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_group_token_index>()
      .indicies()
      .get<by_owner>();

    auto itr = idx.find(std::make_tuple(member, internal_id));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const research_group_token_object& dbs_research_group::add_member_to_research_group(
  const account_name_type& account,
  const research_group_id_type& research_group_id,
  const share_type& share,
  const account_name_type& inviter)
{    
    share_type amount = 0;

    if (inviter != account_name_type()) // invited by individual
    {
        const auto& inviter_rgt = get_research_group_token_by_member(inviter, research_group_id);
        FC_ASSERT(inviter_rgt.amount - share > DEIP_1_PERCENT, 
          "Inviter ${inviter} does not have enough RGT amount to invite ${invitee}. Inviter RGT: ${inviter_rgt}. Invitee RGT ${invitee_rgt}",
          ("inviter", inviter_rgt.owner)("invitee", account)("inviter_rgt", inviter_rgt.amount)("invitee_rgt", share));

        db_impl().modify(inviter_rgt, [&](research_group_token_object& rgt_o) {
            rgt_o.amount -= share;
        });

        amount = share;
    }
    else // invited by research group
    {
        const auto& members_rgt = get_research_group_tokens(research_group_id);
        share_type members_rgt_amount = 0;
        for (auto& wrap : members_rgt) 
        {
            const research_group_token_object& member_rgt = wrap.get();
            db_impl().modify(member_rgt, [&](research_group_token_object& rgt_o) {
                share_type diluted = (share * rgt_o.amount) / DEIP_100_PERCENT;
                if (rgt_o.amount - diluted < DEIP_1_PERCENT)
                {
                    rgt_o.amount = DEIP_1_PERCENT;
                }
                else
                {
                    rgt_o.amount -= diluted;
                }
                members_rgt_amount += rgt_o.amount;
            });
        }

        amount = DEIP_100_PERCENT - members_rgt_amount;
    }

    const research_group_token_object& rgt = 
      db_impl().create<research_group_token_object>(
        [&](research_group_token_object& research_group_token) {
          research_group_token.owner = account;
          research_group_token.research_group_id = research_group_id;
          research_group_token.amount = amount;
        });

      const auto& rgt_s = get_research_group_tokens(research_group_id);
      const share_type total_share = std::accumulate(rgt_s.begin(), rgt_s.end(), share_type(0),
        [&](share_type acc, std::reference_wrapper<const research_group_token_object> wrap) {
          const research_group_token_object& rgt = wrap.get();
          return acc + rgt.amount;
        });

      FC_ASSERT(total_share == DEIP_100_PERCENT, 
        "RGT shares distribution error. Actual: ${actual}. Required: ${required}",
        ("actual", total_share)("required", DEIP_100_PERCENT));

      return rgt;
}

dbs_research_group::research_group_token_refs_type dbs_research_group::remove_member_from_research_group(
  const account_name_type& account,
  const research_group_id_type& research_group_id)
  {
      const research_group_token_object& rgt = get_research_group_token_by_member(account, research_group_id);
      share_type share = rgt.amount;
      db_impl().remove(rgt);

      auto itr_pair = db_impl()
        .get_index<research_group_token_index>()
        .indicies()
        .get<by_research_group>()
        .equal_range(research_group_id);

      auto itr = itr_pair.first;
      const auto itr_end = itr_pair.second;

      account_name_type weakest_member_name = account_name_type();
      share_type weakest_member_share = 0;

      share_type members_rgt_amount = 0;
      while (itr != itr_end)
      {
          db_impl().modify(*itr, [&](research_group_token_object& rgt_o)
          {
              rgt_o.amount = (rgt_o.amount * DEIP_100_PERCENT) / (DEIP_100_PERCENT - share);
              members_rgt_amount += rgt_o.amount;
              if (weakest_member_share == 0 || weakest_member_share > rgt_o.amount)
              {
                  weakest_member_name = rgt_o.owner;
                  weakest_member_share = rgt_o.amount;
              }
          });

          ++itr;
      }

      share_type remainder = DEIP_100_PERCENT - members_rgt_amount;

      if (remainder != 0 && weakest_member_name != account_name_type()) 
      {
          const research_group_token_object& weakest_rgt = get_research_group_token_by_member(weakest_member_name, research_group_id);
          db_impl().modify(weakest_rgt, [&](research_group_token_object& rgt_o)
          {
              rgt_o.amount += remainder;
          });
      }

      const auto& rgt_s = get_research_group_tokens(research_group_id);
      const share_type total_share = std::accumulate(rgt_s.begin(), rgt_s.end(), share_type(0),
        [&](share_type acc, std::reference_wrapper<const research_group_token_object> wrap) {
          const research_group_token_object& rgt = wrap.get();
          return acc + rgt.amount;
        });

      FC_ASSERT(total_share == DEIP_100_PERCENT, 
        "RGT shares distribution error. Actual: ${actual}. Required: ${required}",
        ("actual", total_share)("required", DEIP_100_PERCENT));

    return rgt_s;
  }

const bool dbs_research_group::is_research_group_member(
  const account_name_type& member,
  const research_group_id_type& research_group_id) const
{ 
  const auto& idx = db_impl()
    .get_index<research_group_token_index>()
    .indices()
    .get<by_owner>();

  auto itr = idx.find(std::make_tuple(member, research_group_id));
  return itr != idx.end();
}

dbs_research_group::research_group_token_refs_type dbs_research_group::rebalance_research_group_tokens(
  const research_group_id_type& research_group_id,
  const std::map<account_name_type, share_type> shares)
{
    const auto& rgt_s = get_research_group_tokens(research_group_id);
    FC_ASSERT(shares.size() == rgt_s.size(), "RGT shares should be rebalanced for all research group members at a time");

    share_type total_share = 0;
    for (auto& wrap : rgt_s)
    {
        const research_group_token_object& rgt = wrap.get();
        FC_ASSERT(shares.count(rgt.owner) != 0, "No RGT change for ${a} found", ("a", rgt.owner));
        const share_type share = shares.at(rgt.owner);
        db_impl().modify(rgt, [&](research_group_token_object& rgt_o) { 
          rgt_o.amount = share;
          total_share += rgt_o.amount;
        });
    }

    FC_ASSERT(total_share == DEIP_100_PERCENT, 
      "RGT shares redistribution error. Actual: ${actual}. Required: ${required}",
      ("actual", total_share)("required", DEIP_100_PERCENT));

    return rgt_s;
}


const std::set<account_name_type> dbs_research_group::get_research_group_members(const research_group_id_type& id) const
{
  std::set<account_name_type> ret;

  auto it_pair = db_impl()
    .get_index<research_group_token_index>()
    .indicies()
    .get<by_research_group>()
    .equal_range(id);

  auto it = it_pair.first;
  const auto it_end = it_pair.second;
  while (it != it_end)
  {
    ret.insert(it->owner);
    ++it;
  }
  return ret;
}

const dbs_research_group::research_group_refs_type dbs_research_group::lookup_research_groups(const research_group_id_type& lower_bound, uint32_t limit) const
{
    research_group_refs_type result;

    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indicies()
      .get<by_id>();

    for (auto itr = idx.lower_bound(lower_bound); limit-- && itr != idx.end(); ++itr)
    {
        result.push_back(std::cref(*itr));
    }

    return result;
}


} // namespace chain
} // namespace deip
