#include <deip/chain/services/dbs_security_token.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_security_token::dbs_security_token(database &db)
    : _base_type(db)
{
}

const security_token_object& dbs_security_token::create_security_token(const research_object& research,
                                                                       const external_id_type& external_id,
                                                                       const uint32_t& total_amount)
{
    auto& dgp_service = db_impl().obtain_service<dbs_dynamic_global_properties>();

    const security_token_object& security_token
        = db_impl().create<security_token_object>([&](security_token_object& st_o) {
              st_o.external_id = external_id;
              st_o.research_external_id = research.external_id;
              st_o.total_amount = total_amount;
          });

    const security_token_balance_object& security_token_balance = create_security_token_balance(
        security_token.external_id, security_token.research_external_id, research.research_group, total_amount);

    db_impl().modify(research, [&](research_object& r_o) {
        r_o.security_tokens.insert(std::make_pair(external_id, total_amount));
    });

    dgp_service.create_recent_entity(external_id);

    return security_token;
}

const security_token_balance_object&
dbs_security_token::create_security_token_balance(const external_id_type& security_token_external_id,
                                                  const external_id_type& research_external_id,
                                                  const account_name_type& owner,
                                                  const uint32_t& amount)
{
    FC_ASSERT(amount >= 0, "Security token amount can not be negative");
    const security_token_balance_object& security_token_balance
        = db_impl().create<security_token_balance_object>([&](security_token_balance_object& stb_o) {
              stb_o.security_token_external_id = security_token_external_id;
              stb_o.research_external_id = research_external_id;
              stb_o.owner = owner;
              stb_o.amount = amount;
          });

    return security_token_balance;
}


void dbs_security_token::transfer_security_token(const account_name_type& from,
                                                 const account_name_type& to,
                                                 const external_id_type& security_token_external_id,
                                                 const uint32_t& amount)
{
    const auto& research_service = db_impl().obtain_service<dbs_research>();
    const auto& security_token_opt = get_security_token_if_exists(security_token_external_id);

    FC_ASSERT(security_token_opt.valid(), "Security token ${1} does not exist", ("1", security_token_external_id));
    
    const security_token_object& security_token = *security_token_opt;
    const auto& research = research_service.get_research(security_token.research_external_id);

    const auto& from_balance_opt = get_security_token_balance_if_exists(from, security_token.external_id);
    FC_ASSERT(from_balance_opt.valid(), "Security token ${1} balance does not exist for ${2} account",
      ("1", security_token.external_id)("2", from));

    const security_token_balance_object& from_balance = *from_balance_opt;

    FC_ASSERT(from_balance.amount >= amount, "Security token ${1} balance for account ${2} is not enough (${3} units) to transfer ${4} units",
      ("1", security_token.external_id)("2", from)("3", from_balance.amount)("4", amount));

    const auto& to_balance_opt = get_security_token_balance_if_exists(to, security_token.external_id);

    if (!to_balance_opt.valid())
    {
        create_security_token_balance(security_token.external_id, research.external_id, to, amount);
    }
    else
    {
        const security_token_balance_object& to_balance = *to_balance_opt;
        db_impl().modify(to_balance, [&](security_token_balance_object& stb_o) { stb_o.amount += amount; });
    }

    db_impl().modify(from_balance, [&](security_token_balance_object& stb_o) { 
        stb_o.amount -= amount; 
    });

    if (from_balance.amount == 0)
    {
        db_impl().remove(from_balance);
    }

    const auto& security_token_balances = get_security_token_balances(security_token.external_id);
    const uint32_t total_amount = std::accumulate(
      security_token_balances.begin(), security_token_balances.end(), uint32_t(0),
      [&](uint32_t acc, const security_token_balance_object& st_o) {
        return acc + st_o.amount;
      });

    const uint32_t issued_amount = security_token.total_amount;

    FC_ASSERT(total_amount == issued_amount, "Total amount ${1} is more than issued amount ${2} for security token ${3}", 
      ("1", total_amount)("2", issued_amount)("3", security_token_external_id));        
}


const security_token_object& dbs_security_token::get_security_token(const external_id_type& external_id) const
{
    try
    {
        return db_impl().get<security_token_object, by_external_id>(external_id);
    }
    FC_CAPTURE_AND_RETHROW((external_id))
}


const dbs_security_token::security_token_optional_ref_type
dbs_security_token::get_security_token_if_exists(const external_id_type& security_token_external_id) const
{
    security_token_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<security_token_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(security_token_external_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}


const dbs_security_token::security_token_refs_type
dbs_security_token::get_security_tokens_by_research(const external_id_type& research_external_id) const
{
    security_token_refs_type ret;

    auto it_pair = db_impl()
      .get_index<security_token_index>()
      .indicies()
      .get<by_research>()
      .equal_range(research_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const security_token_balance_object&
dbs_security_token::get_security_token_balance(const account_name_type& owner, const external_id_type& security_token_external_id) const
{
    try
    {
        return db_impl().get<security_token_balance_object, by_owner_and_security_token>(std::make_tuple(owner, security_token_external_id));
    }

    FC_CAPTURE_AND_RETHROW((owner)(security_token_external_id))
}


const dbs_security_token::security_token_balance_optional_ref_type
dbs_security_token::get_security_token_balance_if_exists(const account_name_type& owner,
                                                         const external_id_type& security_token_external_id) const
{
    security_token_balance_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<security_token_balance_index>()
      .indicies()
      .get<by_owner_and_security_token>();

    auto itr = idx.find(std::make_tuple(owner, security_token_external_id));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}


const dbs_security_token::security_token_balance_refs_type
dbs_security_token::get_security_token_balances(const external_id_type& security_token_external_id) const
{
    security_token_balance_refs_type ret;

    auto it_pair = db_impl()
      .get_index<security_token_balance_index>()
      .indicies()
      .get<by_security_token>()
      .equal_range(security_token_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}


const dbs_security_token::security_token_balance_refs_type
dbs_security_token::get_security_token_balances_by_owner(const account_name_type& owner) const
{
    security_token_balance_refs_type ret;

    auto it_pair = db_impl()
      .get_index<security_token_balance_index>()
      .indicies()
      .get<by_owner>()
      .equal_range(owner);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

const dbs_security_token::security_token_balance_refs_type
dbs_security_token::get_security_token_balances_by_research(const external_id_type& research_external_id) const
{
    security_token_balance_refs_type ret;

    auto it_pair = db_impl()
      .get_index<security_token_balance_index>()
      .indicies()
      .get<by_research>()
      .equal_range(research_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}


const dbs_security_token::security_token_balance_refs_type
dbs_security_token::get_security_token_balances_by_owner_and_research(const account_name_type& owner, const external_id_type& research_external_id) const
{
    security_token_balance_refs_type ret;

    auto it_pair = db_impl()
      .get_index<security_token_balance_index>()
      .indicies()
      .get<by_owner_and_research>()
      .equal_range(boost::make_tuple(owner, research_external_id));

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

} //namespace chain
} //namespace deip