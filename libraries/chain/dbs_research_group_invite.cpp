#include <deip/chain/dbs_research_group_invite.hpp>
#include <deip/chain/database.hpp>


namespace deip{
namespace chain{

dbs_research_group_invite::dbs_research_group_invite(database &db) : _base_type(db)
{
}

const research_group_invite_object& dbs_research_group_invite::create(const account_name_type& account_name,
                                                                      const research_group_id_type& research_group_id)
{
    const auto& new_research_group_invite = db_impl().create<research_group_invite_object>([&](research_group_invite_object& rgi_o) {
        rgi_o.account_name = account_name;
        rgi_o.research_group_id = research_group_id;
    });

    return new_research_group_invite;
}

dbs_research_group_invite::research_group_invite_refs_type
    dbs_research_group_invite::get_research_group_invite_by_account_name_and_research_group_id(const account_name_type& account_name,
                                                                                               const research_group_id_type& research_group_id)
{
    research_group_invite_refs_type ret;

    auto it_pair
            = db_impl().get_index<research_group_invite_index>().indicies().get<by_account_and_research_group_id>().equal_range(std::make_tuple(account_name, research_group_id));

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

}
}