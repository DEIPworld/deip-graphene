#include <deip/chain/dbs_research_token_sale_contribution.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_research_token_sale_contribution::dbs_research_token_sale_contribution(database& db)
    : _base_type(db)
{
}

const research_token_sale_contribution_object&
    dbs_research_token_sale_contribution::create_research_token_sale_contributiont(const research_id_type& research_id,
                                                                                   const account_name_type& owner,
                                                                                   const fc::time_point contribution_time,
                                                                                   const deip::chain::share_type amount)
{
    const auto& new_research_token_sale_contribution
            = db_impl().create<research_token_sale_contribution_object>([&](research_token_sale_contribution_object& research_token_sale_contribution) {
                research_token_sale_contribution.research_id = research_id;
                research_token_sale_contribution.owner = owner;
                research_token_sale_contribution.contribution_time = contribution_time;
                research_token_sale_contribution.amount = amount;
            });

    return new_research_token_sale_contribution;
}

const research_token_sale_contribution_object&
    dbs_research_token_sale_contribution::get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type& id) const
{
    return db_impl().get<research_token_sale_contribution_object, by_id>(id);
}

dbs_research_token_sale_contribution::research_token_sale_contribution_refs_type
    dbs_research_token_sale_contribution::get_research_token_sale_contribution_by_research_id(const research_id_type &research_id) const
{
    research_token_sale_contribution_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_sale_contribution_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_token_sale_contribution::research_token_sale_contribution_refs_type
    dbs_research_token_sale_contribution::get_research_token_sale_contribution_by_account_name(const account_name_type& owner) const
{
    research_token_sale_contribution_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_sale_contribution_index>().indicies().get<by_owner>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

} // namespace chain
} // namespace deip

