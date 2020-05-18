#include <deip/chain/services/dbs_research_group_invite.hpp>
#include <deip/chain/database/database.hpp>

namespace deip{
namespace chain{

dbs_research_group_invite::dbs_research_group_invite(database &db) : _base_type(db)
{
}

const research_group_invite_object& dbs_research_group_invite::create(const account_name_type& account_name,
                                                                      const research_group_id_type& research_group_id,
                                                                      const share_type& research_group_token_amount,
                                                                      const std::string& cover_letter,
                                                                      const account_name_type& token_source,
                                                                      const bool& is_head)
{
    const auto& new_research_group_invite = db_impl().create<research_group_invite_object>([&](research_group_invite_object& rgi_o) {
        rgi_o.account_name = account_name;
        rgi_o.research_group_id = research_group_id;
        rgi_o.research_group_token_amount = research_group_token_amount;
        rgi_o.expiration_time = _get_now() + DAYS_TO_SECONDS(14);
        fc::from_string(rgi_o.cover_letter, cover_letter);
        rgi_o.token_source = token_source;
        rgi_o.is_head = is_head;
    });

    return new_research_group_invite;
}

const research_group_invite_object& dbs_research_group_invite::get_research_group_invite(const research_group_invite_id_type& research_group_invite_id) const
{
    const auto& idx = db_impl()
      .get_index<research_group_invite_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(research_group_invite_id);
    FC_ASSERT(itr != idx.end(), "Research group invite ${i} does not exist", ("i", research_group_invite_id));

    return *itr;
}

const dbs_research_group_invite::research_group_invite_optional_ref_type
dbs_research_group_invite::get_research_group_invite_if_exists(const research_group_invite_id_type& research_group_invite_id) const
{
    research_group_invite_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_group_invite_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(research_group_invite_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const bool dbs_research_group_invite::research_group_invite_exists(const research_group_invite_id_type& research_group_invite_id) const
{
    const auto& idx = db_impl()
      .get_index<research_group_invite_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(research_group_invite_id);
    return itr != idx.end();
}

const research_group_invite_object& dbs_research_group_invite::get_research_group_invite_by_account_and_research_group(const account_name_type& account_name,
                                                                                                                       const research_group_id_type& research_group_id) const
{
    const auto& idx = db_impl()
      .get_index<research_group_invite_index>()
      .indicies()
      .get<by_account_and_research_group_id>();

    auto itr = idx.find(std::make_tuple(account_name, research_group_id));
    FC_ASSERT(itr != idx.end(), "Research group invite for ${a} in researh group ${rg} does not exist", ("a", account_name)("rg", research_group_id));

    return *itr;
}

const dbs_research_group_invite::research_group_invite_optional_ref_type
dbs_research_group_invite::get_research_group_invite_by_account_and_research_group_if_exists(const account_name_type& account_name,
                                                                                             const research_group_id_type& research_group_id) const
{
    research_group_invite_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_group_invite_index>()
      .indicies()
      .get<by_account_and_research_group_id>();

    auto itr = idx.find(std::make_tuple(account_name, research_group_id));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_research_group_invite::research_group_invite_refs_type
    dbs_research_group_invite::get_research_group_invites_by_account_name(const account_name_type& account_name) const
{
    research_group_invite_refs_type ret;

    auto it_pair = db_impl().get_index<research_group_invite_index>().indicies().get<by_account_name>().equal_range(account_name);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_group_invite::research_group_invite_refs_type
    dbs_research_group_invite::get_research_group_invites_by_research_group_id(const research_group_id_type& research_group_id) const
{
    research_group_invite_refs_type ret;

    auto it_pair = db_impl().get_index<research_group_invite_index>().indicies().get<by_research_group_id>().equal_range(research_group_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_research_group_invite::clear_expired_invites()
{
    const auto& invite_expiration_index = db_impl().get_index<research_group_invite_index>().indices().get<by_expiration_time>();

    auto block_time = db_impl().head_block_time();
    auto invite_itr = invite_expiration_index.upper_bound(block_time);

    while (invite_expiration_index.begin() != invite_itr && is_expired(*invite_expiration_index.begin()))
    {
        db_impl().remove(*invite_expiration_index.begin());
    }
}

bool dbs_research_group_invite::is_expired(const research_group_invite_object& invite)
{
    return _get_now() > invite.expiration_time;
}

void dbs_research_group_invite::remove(const research_group_invite_object& invite)
{
    db_impl().remove(invite);
}

}
}