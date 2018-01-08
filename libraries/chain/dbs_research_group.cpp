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

const research_group_token_object& dbs_research_group_token::get_research_group_token_by_account_type(account_name_type account_t) const {
    return db_impl().get<research_group_token_object, by_owner>(account_t);
}

const research_group_token_object& dbs_research_group_token::create_research_group_token(const research_group_id_type research_group,
                                                                                         const share_type share,
                                                                                         const account_name_type account_name) {
    const research_group_token_object& new_research_group_token = (const research_group_token_object &) db_impl().create<research_group_token_object>([&](research_group_token_object& research_group_token) {
                research_group_token.research_group = research_group;
                research_group_token.amount = share;
                research_group_token.owner = account_name;
    });

    return new_research_group_token;
}

} // namespace chain
} // namespace deip