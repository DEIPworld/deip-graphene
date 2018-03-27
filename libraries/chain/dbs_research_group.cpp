#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_research_group::dbs_research_group(database& db)
    : _base_type(db)
{
}

const research_group_object& dbs_research_group::get_research_group(const research_group_id_type& id) const {
    return db_impl().get<research_group_object, by_id>(id);
}

const research_group_object& dbs_research_group::get_research_group_by_permlink(const fc::string& permlink) const {
    return db_impl().get<research_group_object, by_permlink>(permlink);
}

const research_group_object& dbs_research_group::create_research_group(const string& permlink,
                                                                       const string& description,
                                                                       const share_type quorum_percent) {
    const research_group_object& new_research_group = db_impl().create<research_group_object>([&](research_group_object& research_group) {
        research_group.permlink = permlink;
        research_group.description = description;
        research_group.funds = 0;
        research_group.quorum_percent = quorum_percent;
        research_group.total_tokens_amount = DEIP_100_PERCENT;
    });

    return new_research_group;
}

void dbs_research_group::change_quorum(const uint32_t quorum_percent, const research_group_id_type& research_group_id)
{
    check_research_group_existence(research_group_id);
    const research_group_object& research_group = get_research_group(research_group_id);
    db_impl().modify(research_group, [&](research_group_object& rg) { rg.quorum_percent = quorum_percent; });
}

void dbs_research_group::check_research_group_existence(const research_group_id_type& research_group_id) const
{
    const auto& idx = db_impl().get_index<research_group_index>().indices().get<by_id>();
    FC_ASSERT(idx.find(research_group_id) != idx.cend(), "Group \"${1}\" does not exist.", ("1", research_group_id));
}

void dbs_research_group::check_research_group_existence_by_permlink(const string& permlink) const
{
    const auto& idx = db_impl().get_index<research_group_index>().indices().get<by_permlink>();
    FC_ASSERT(idx.find(permlink) != idx.cend(), "Group by permlink: \"${1}\" does not exist.", ("1", permlink));
}

const research_group_token_object& dbs_research_group::get_research_group_token_by_id(const research_group_token_id_type& id) const {
    return db_impl().get<research_group_token_object>(id);
}

dbs_research_group::research_group_token_refs_type dbs_research_group::get_research_group_tokens_by_account_name(const account_name_type
                                                                                                                 &account_name) const
{
    research_group_token_refs_type ret;

    auto it_pair = db_impl().get_index<research_group_token_index>().indicies().get<by_owner>().equal_range(account_name);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_group::research_group_token_refs_type
dbs_research_group::get_research_group_tokens(const research_group_id_type &research_group_id) const
{
    research_group_token_refs_type ret;

    auto it_pair = db_impl().get_index<research_group_token_index>().indicies().get<by_research_group>().equal_range(research_group_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_group_token_object& dbs_research_group::get_research_group_token_by_account_and_research_group_id(const account_name_type &account,
                                                                                           const research_group_id_type &research_group_id) const  {
    return db_impl().get<research_group_token_object, by_owner>(boost::make_tuple(account, research_group_id));
}

const research_group_token_object& dbs_research_group::create_research_group_token(const research_group_id_type& research_group_id,
                                                                                   const share_type amount,
                                                                                   const account_name_type& owner) {
    const research_group_token_object& new_research_group_token = db_impl()
            .create<research_group_token_object>([&](research_group_token_object& research_group_token) {
        research_group_token.research_group_id = research_group_id;
        research_group_token.amount = amount * DEIP_1_PERCENT;
        research_group_token.owner = owner;
    });

    return new_research_group_token;
}

void dbs_research_group::check_research_group_token_existence(const account_name_type& account,
                                                        const research_group_id_type& research_group_id) const
{
    const auto& idx = db_impl().get_index<research_group_token_index>().indices().get<by_owner>();

    FC_ASSERT(idx.find(boost::make_tuple(account, research_group_id)) != idx.cend(), "Account \"${1}\" does not exist in \"${2}\" group.",
              ("1", account)("2", research_group_id));
}

void dbs_research_group::remove_token(const account_name_type& account,
                                      const research_group_id_type& research_group_id)
{
    check_research_group_token_existence(account, research_group_id);
    const research_group_token_object& token = get_research_group_token_by_account_and_research_group_id(account, research_group_id);
    db_impl().remove(token);
}

const research_group_object& dbs_research_group::increase_research_group_funds(const research_group_id_type& research_group_id,
                                                                               const share_type deips)
{
    const research_group_object& research_group = get_research_group(research_group_id);
    db_impl().modify(research_group, [&](research_group_object& rg) { rg.funds += deips; });

    return research_group;
}

const research_group_object& dbs_research_group::decrease_research_group_funds(const research_group_id_type& research_group_id,
                                                                               const share_type deips)
{
    const research_group_object& research_group = get_research_group(research_group_id);
    FC_ASSERT(research_group.funds > deips, "Not enough funds");
    db_impl().modify(research_group, [&](research_group_object& rg) { rg.funds -= deips; });

    return research_group;
}

void dbs_research_group::adjust_research_group_tokens_amount(const research_group_id_type& research_group_id,
                                                             const share_type delta)
{
    auto it_pair = db_impl().get_index<research_group_token_index>().indicies().get<by_research_group>().equal_range(research_group_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        db_impl().modify(*it, [&](research_group_token_object& rgt) { rgt.amount += delta * DEIP_1_PERCENT; });
        ++it;
    }

}

const research_group_token_object& dbs_research_group::set_new_research_group_token_amount(const research_group_id_type& research_group_id,
                                                                                           const account_name_type& owner,
                                                                                           const share_type new_amount)
{
    const research_group_token_object& research_group_token = get_research_group_token_by_account_and_research_group_id(owner, research_group_id);
    db_impl().modify(research_group_token, [&](research_group_token_object& rgo) { rgo.amount = new_amount * DEIP_1_PERCENT; });

    return research_group_token;
}

} // namespace chain
} // namespace deip
