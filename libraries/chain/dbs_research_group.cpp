#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/database.hpp>




namespace deip {
namespace chain {

dbs_research_group::dbs_research_group(database& db)
    : _base_type(db)
{
}

const research_group_object& dbs_research_group::get_research_group(research_group_id_type id) const {
    return db_impl().get<research_group_object>(id);
}

const research_group_object& dbs_research_group::create_research_group(const string permlink,
                                                                       const string description) {
    const research_group_object& new_research_group = db_impl().create<research_group_object>([&](research_group_object& research_group) {
        fc::from_string(research_group.permlink, permlink);
        fc::from_string(research_group.desciption, description);
    });

    return new_research_group;
}

void dbs_research_group::change_quorum_group_object(u_int16_t quorum_percent, research_group_id_type research_group_id)
{
    FC_ASSERT(group_exists(research_group_id), "Group does not exist.");

    const research_group_object& research_group = db().obtain_service<dbs_research_group>().get_research_group(research_group_id);

    db_impl().modify(research_group, [&](research_group_object& rg) { rg.quorum_percent = quorum_percent; });
}

bool dbs_research_group::group_exists(research_group_id_type research_group_id) const
{
    const auto& idx = db_impl().get_index<research_group_index>().indices().get<by_id>();
    return idx.find(research_group_id) != idx.cend();
}

dbs_research_group_token::dbs_research_group_token(database& db)
    : _base_type(db)
{
}

const research_group_token_object& dbs_research_group_token::get_research_group_token(research_group_token_id_type id) const {
    return db_impl().get<research_group_token_object>(id);
}

const research_group_token_object& dbs_research_group_token::get_research_group_token(const account_name_type account_name, 
                                                                                      const research_group_id_type research_group_id) const {
    return db_impl().get<research_group_token_object, by_owner>(boost::make_tuple(account_name, research_group_id));
}

const research_group_token_object& dbs_research_group_token::create_research_group_token(const research_group_id_type research_group_id,
                                                                                         const share_type amount,
                                                                                         const account_name_type account_name) {
    const research_group_token_object& new_research_group_token = db_impl().create<research_group_token_object>([&](research_group_token_object& research_group_token) {
                research_group_token.research_group = research_group_id;
                research_group_token.amount = amount;
                research_group_token.owner = account_name;
    });

    const research_group_object& research_group = db().obtain_service<dbs_research_group>().get_research_group(research_group_id);

    db_impl().modify(research_group, [&](research_group_object& rg) { rg.total_tokens_amount += amount; });

    return new_research_group_token;
}

bool dbs_research_group_token::token_exists(const account_name_type& account_name, 
                                            research_group_id_type research_group_id) const
{
    const auto& idx = db_impl().get_index<research_group_token_index>().indices().get<by_owner>();
    return idx.find(boost::make_tuple(account_name, research_group_id)) != idx.cend();
}

void dbs_research_group_token::remove_token_object(const account_name_type& account_name,
                                                   research_group_id_type research_group_id)
{
    FC_ASSERT(token_exists(account_name, research_group_id), "Token does not exist");

    const research_group_token_object& token = get_research_group_token(account_name, research_group_id);

    const research_group_object& research_group = db().obtain_service<dbs_research_group>().get_research_group(research_group_id);

    db_impl().modify(research_group, [&](research_group_object& rg) { rg.total_tokens_amount -= token.amount; });

    db_impl().remove(token);
}

void dbs_research_group_token::add_share_to_research_group_token(const share_type amount, 
                                                                 const research_group_token_object& research_group_token) {
    db_impl().modify(research_group_token, [&](research_group_token_object& rgt) { rgt.amount += amount; });

    const research_group_object& research_group = db().obtain_service<dbs_research_group>().get_research_group(research_group_token.research_group);

    db_impl().modify(research_group, [&](research_group_object& rg) { rg.total_tokens_amount += amount; });
}

} // namespace chain
} // namespace deip