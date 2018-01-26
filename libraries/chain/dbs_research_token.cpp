#include <deip/chain/dbs_research_token.hpp>
#include <deip/chain/database.hpp>

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

const research_token_object& dbs_research_token::get_research_token(const research_token_id_type &id) const
{
    return db_impl().get<research_token_object>(id);
}

dbs_research_token::research_token_refs_type dbs_research_token::get_research_tokens_by_account_name(const account_name_type &account_name) const
{
    research_token_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_index>().indicies().get<by_account_name>().equal_range(account_name);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_token::research_token_refs_type dbs_research_token::get_research_tokens_by_research_id(const research_id_type &research_id) const
{
    research_token_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_token_object& dbs_research_token::get_research_token_by_account_name_and_research_id(const account_name_type &account_name,
                                                                                         const research_id_type &research_id) const
{
    return db_impl().get<research_token_object, by_account_name_and_research_id>(boost::make_tuple(account_name, research_id));
}

} //namespace chain
} //namespace deip
