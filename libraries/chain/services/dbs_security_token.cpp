#include <deip/chain/services/dbs_security_token.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_security_token::dbs_security_token(database &db)
    : _base_type(db)
{
}

const security_token_object& dbs_security_token::create_security_token(const research_object& research,
                                                                       const external_id_type& security_token_external_id,
                                                                       const uint64_t& amount)
{
    const auto& research_group_service = db_impl().obtain_service<dbs_research_group>();
    const auto& research_group = research_group_service.get_research_group(research.research_group_id);

    const security_token_object& security_token
        = db_impl().create<security_token_object>([&](security_token_object& st_o) {
              st_o.owner = research_group.account;
              st_o.research_external_id = research.external_id;
              st_o.security_token_external_id = security_token_external_id;
              st_o.amount = amount;
          });

    db_impl().modify(research, [&](research_object& r_o) {
        r_o.security_tokens.insert(std::make_pair(security_token_external_id, amount));
    });

    return security_token;
}

void dbs_security_token::transfer_security_token(const account_name_type& from,
                                                 const account_name_type& to,
                                                 const external_id_type& security_token_external_id,
                                                 const uint64_t& amount)
{
    const auto& research_service = db_impl().obtain_service<dbs_research>();

    const auto& from_security_token_opt = get_security_token_by_owner_if_exists(from, security_token_external_id);
    FC_ASSERT(from_security_token_opt.valid(), "Security token ${1} balance does not exist for ${2} account",
      ("1", security_token_external_id)("2", from));

    const security_token_object& from_security_token = *from_security_token_opt;
    const auto& research = research_service.get_research(from_security_token.research_external_id);

    FC_ASSERT(from_security_token.amount >= amount, "Security token ${1} balance for account ${2} is not enough (${3} units) to transfer ${4} units",
      ("1", security_token_external_id)("2", from)("3", from_security_token.amount)("4", amount));
    
    const auto& to_security_token_opt = get_security_token_by_owner_if_exists(to, security_token_external_id);

    if (!to_security_token_opt.valid())
    {
        db_impl().create<security_token_object>([&](security_token_object& st_o) {
            st_o.owner = to;
            st_o.security_token_external_id = from_security_token.security_token_external_id;
            st_o.research_external_id = from_security_token.research_external_id;
            st_o.amount = amount;
        });
    }
    else
    {
        const security_token_object& to_security_token = *to_security_token_opt;
        db_impl().modify(to_security_token, [&](security_token_object& st_o) { 
            st_o.amount += amount; 
        });
    }

    db_impl().modify(from_security_token, [&](security_token_object& st_o) { 
        st_o.amount -= amount; 
    });

    if (from_security_token.amount == 0)
    {
        db_impl().remove(from_security_token);
    }

    const auto& security_tokens = get_security_tokens_by_research(research.external_id);
    const uint64_t total_amount = std::accumulate(
      security_tokens.begin(), security_tokens.end(), uint64_t(0),
      [&](uint64_t acc, const security_token_object& security_token) {
        return acc + security_token.amount;
      });

    const uint64_t issued_amount = research.security_tokens.at(security_token_external_id);

    FC_ASSERT(total_amount == issued_amount, "Total amount ${1} is more than issued amount ${2} for security token ${3}", 
      ("1", total_amount)("2", issued_amount)("3", security_token_external_id));        
}

const security_token_object& dbs_security_token::get_security_token(const security_token_id_type& id) const
{
    try
    {
        return db_impl().get<security_token_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}


const dbs_security_token::security_token_refs_type
dbs_security_token::get_security_tokens(const external_id_type& security_token_external_id) const
{
    security_token_refs_type ret;

    auto it_pair = db_impl().get_index<security_token_index>().indicies().get<by_security_token>().equal_range(security_token_external_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}


const dbs_security_token::security_token_refs_type
dbs_security_token::get_security_tokens_by_owner(const account_name_type& owner) const
{
    security_token_refs_type ret;

    auto it_pair = db_impl().get_index<security_token_index>().indicies().get<by_owner>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}


const dbs_security_token::security_token_refs_type
dbs_security_token::get_security_tokens_by_research(const external_id_type& research_external_id) const
{
    security_token_refs_type ret;

    auto it_pair = db_impl().get_index<security_token_index>().indicies().get<by_research>().equal_range(research_external_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const security_token_object&
dbs_security_token::get_security_token_by_owner(const account_name_type& owner,
                                                const external_id_type& security_token_external_id) const
{
    try
    {
        return db_impl().get<security_token_object, by_owner_and_security_token>(std::make_tuple(owner, security_token_external_id));
    }

    FC_CAPTURE_AND_RETHROW((owner)(security_token_external_id))
}


const dbs_security_token::security_token_optional_ref_type
dbs_security_token::get_security_token_by_owner_if_exists(const account_name_type& owner,
                                                          const external_id_type& security_token_external_id) const
{
    security_token_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<security_token_index>()
      .indicies()
      .get<by_owner_and_security_token>();

    auto itr = idx.find(std::make_tuple(owner, security_token_external_id));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}


const bool dbs_security_token::security_token_exists_by_owner(const account_name_type& owner,
                                                              const external_id_type& security_token_external_id) const
{
    const auto& idx = db_impl().get_index<security_token_index>().indices().get<by_owner_and_security_token>();
    return idx.find(std::make_tuple(owner, security_token_external_id)) != idx.end();
}

} //namespace chain
} //namespace deip