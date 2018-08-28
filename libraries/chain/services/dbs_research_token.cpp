#include <deip/chain/services/dbs_research_token.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_research_token::dbs_research_token(database &db)
    : _base_type(db)
{
}

const research_token_object& dbs_research_token::create_research_token(const account_name_type &owner,
                                                                       const deip::chain::share_type amount,
                                                                       const research_id_type &research_id)
{
    const research_token_object& new_research_token = db_impl().create<research_token_object>([&](research_token_object& research_token) {
        research_token.account_name = owner;
        research_token.amount = amount;
        research_token.research_id = research_id;
    });

    return new_research_token;
}

void dbs_research_token::increase_research_token_amount(const research_token_object& research_token, const share_type delta)
{
    FC_ASSERT((delta >= 0), "Cannot update research token amount (delta < 0)");
    db_impl().modify(research_token, [&](research_token_object& rt_o) { rt_o.amount += delta; });
}

void dbs_research_token::decrease_research_token_amount(const research_token_object& research_token, const share_type delta)
{
    FC_ASSERT((research_token.amount - delta >= 0), "Cannot update research token amount (result amount < 0)");
    db_impl().modify(research_token, [&](research_token_object& rt_o) { rt_o.amount -= delta; });
}

const research_token_object& dbs_research_token::get(const research_token_id_type &id) const
{
    try {
        return db_impl().get<research_token_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

dbs_research_token::research_token_refs_type dbs_research_token::get_by_owner(const account_name_type &owner) const
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

dbs_research_token::research_token_refs_type dbs_research_token::get_by_research(const research_id_type &research_id) const
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

const research_token_object& dbs_research_token::get_by_owner_and_research(const account_name_type &owner,
                                                                                         const research_id_type &research_id) const
{
    try {
        return db_impl().get<research_token_object, by_account_name_and_research_id>(
                boost::make_tuple(owner, research_id));
    }
    FC_CAPTURE_AND_RETHROW((owner)(research_id))
}

void dbs_research_token::check_existence_by_owner_and_research(const account_name_type& owner,
                                                                                        const research_id_type& research_id)
{
    const auto& idx = db_impl().get_index<research_token_index>().indices().get<by_account_name_and_research_id>();
    
    FC_ASSERT(idx.find(boost::make_tuple(owner, research_id)) != idx.cend(), "Research token doesnt exist");

}

bool dbs_research_token::exists_by_owner_and_research(const account_name_type& owner,
                                                                                  const research_id_type& research_id)
{
    const auto& idx = db_impl().get_index<research_token_index>().indices().get<by_account_name_and_research_id>();

    return idx.find(boost::make_tuple(owner, research_id)) != idx.cend();
}

} //namespace chain
} //namespace deip
