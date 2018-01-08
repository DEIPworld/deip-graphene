#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/database.hpp>

#include <tuple>

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

dbs_research_group_token::dbs_research_group_token(database& db)
    : _base_type(db)
{
}

const research_group_token_object& dbs_research_group_token::get_research_group_token(research_group_token_id_type id) const {
    return db_impl().get<research_group_token_object>(id);
}

const research_group_token_object& dbs_research_group_token::create_research_group_token(const research_group_id_type research_group_id,
                                                                                         const share_type amount,
                                                                                         const account_name_type account_name) {
    const research_group_token_object& new_research_group_token = (const research_group_token_object &) db_impl().create<research_group_token_object>([&](research_group_token_object& research_group_token) {
                research_group_token.research_group = research_group_id;
                research_group_token.amount = amount;
                research_group_token.owner = account_name;
    });

    const research_group_object& research_group = db().obtain_service<dbs_research_group>().get_research_group(research_group_id);

    db_impl().modify(research_group, [&](research_group_object& rg) { rg.total_tokens_amount += amount; });

    return new_research_group_token;
}

void dbs_research_group_token::add_share_to_research_group_token(const share_type amount, 
                                                                 const research_group_token_object& research_group_token) {
    db_impl().modify(research_group_token, [&](research_group_token_object& rgt) { rgt.amount += amount; });

    const research_group_object& research_group = db().obtain_service<dbs_research_group>().get_research_group(research_group_token.research_group);

    db_impl().modify(research_group, [&](research_group_object& rg) { rg.total_tokens_amount += amount; });
}

} // namespace chain
} // namespace deip