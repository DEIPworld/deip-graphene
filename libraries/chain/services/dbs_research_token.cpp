#include <deip/chain/services/dbs_research_token.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_research_token::dbs_research_token(database &db)
    : _base_type(db)
{
}

const research_token_object& dbs_research_token::create_research_token(
  const account_name_type& owner,
  const research_object& research,
  const share_type& amount,
  const bool& is_compensation)
{
    const research_token_object& research_token = db_impl().create<research_token_object>([&](research_token_object& rt_o) {
        rt_o.account_name = owner;
        rt_o.amount = amount;
        rt_o.research_id = research.id;
        rt_o.research_external_id = research.external_id;
        rt_o.is_compensation = is_compensation;
    });

    return research_token;
}


const research_token_object& dbs_research_token::get(const research_token_id_type &id) const
{
    try {
        return db_impl().get<research_token_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_research_token::research_token_optional_ref_type
dbs_research_token::get_research_token_if_exists(const research_token_id_type& id) const
{
    research_token_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<research_token_index>()
            .indicies()
            .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_research_token::research_token_refs_type dbs_research_token::get_account_balance_by_owner(const account_name_type &owner) const
{
    research_token_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_index>().indicies().get<by_account_name>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end){
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_token::research_token_refs_type dbs_research_token::get_by_research(const research_id_type& research_id) const
{
    research_token_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end){
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_token_object& dbs_research_token::get_by_owner_and_research(
  const account_name_type& owner,
  const research_id_type& research_id) const
{
    try { return db_impl().get<research_token_object, by_account_name_and_research_id>(std::make_tuple(owner, research_id)); }
    FC_CAPTURE_AND_RETHROW((owner)(research_id))
}

const dbs_research_token::research_token_optional_ref_type
dbs_research_token::get_research_token_by_owner_and_research_if_exists(const account_name_type &owner,
                                                                       const research_id_type &research_id) const
{
    research_token_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<research_token_index>()
            .indicies()
            .get<by_account_name_and_research_id>();

    auto itr = idx.find(std::make_tuple(owner, research_id));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

void dbs_research_token::check_existence_by_owner_and_research(const account_name_type& owner,
                                                                                        const research_id_type& research_id)
{
    const auto& idx = db_impl().get_index<research_token_index>().indices().get<by_account_name_and_research_id>();
    
    FC_ASSERT(idx.find(boost::make_tuple(owner, research_id)) != idx.cend(), "Research token doesnt exist");

}

const bool dbs_research_token::exists_by_owner_and_research(
  const account_name_type& owner,
  const research_id_type& research_id) const
{
    const auto& idx = db_impl()
      .get_index<research_token_index>()
      .indices()
      .get<by_account_name_and_research_id>();

    return idx.find(std::make_tuple(owner, research_id)) != idx.end();
}

} //namespace chain
} //namespace deip