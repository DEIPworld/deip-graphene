#include <deip/chain/services/dbs_research_token_sale.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_research_token_sale::dbs_research_token_sale(database& db)
    : _base_type(db)
{
}

const research_token_sale_object& dbs_research_token_sale::start(const research_id_type &research_id,
                                                                 const fc::time_point_sec start_time,
                                                                 const fc::time_point_sec end_time,
                                                                 const share_type& balance_tokens,
                                                                 const asset soft_cap,
                                                                 const asset hard_cap)
{
    FC_ASSERT(start_time >= db_impl().head_block_time(), "Start time must be >= current time");
    FC_ASSERT(end_time > start_time, "End time must be >= start time");
    const research_token_sale_object& new_research_token_sale
        = db_impl().create<research_token_sale_object>([&](research_token_sale_object& research_token_sale) {
              research_token_sale.research_id = research_id;
              research_token_sale.start_time = start_time;
              research_token_sale.end_time = end_time;
              research_token_sale.total_amount = asset(0, DEIP_SYMBOL);
              research_token_sale.balance_tokens = balance_tokens;
              research_token_sale.soft_cap = soft_cap;
              research_token_sale.hard_cap = hard_cap;
              research_token_sale.status = research_token_sale_status::token_sale_inactive;
          });

    return new_research_token_sale;
}

dbs_research_token_sale::research_token_sale_refs_type dbs_research_token_sale::get_all() const
{
    research_token_sale_refs_type ret;

    const auto& idx = db_impl().get_index<research_token_sale_index>().indicies().get<by_id>();
    auto it = idx.lower_bound(0);
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_token_sale_object& dbs_research_token_sale::get_by_id(const research_token_sale_id_type &id) const
{
    try {
        return db_impl().get<research_token_sale_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const research_token_sale_object& dbs_research_token_sale::get_by_research_id(const research_id_type &research_id) const
{
    try {
        return db_impl().get<research_token_sale_object, by_research_id>(research_id);
    }
    FC_CAPTURE_AND_RETHROW((research_id))
}

dbs_research_token_sale::research_token_sale_refs_type
dbs_research_token_sale::get_by_end_time(const fc::time_point_sec &end_time) const
{
    research_token_sale_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_sale_index>().indicies().get<by_end_time>().equal_range(end_time);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_research_token_sale::check_research_token_sale_existence(const research_token_sale_id_type& id) const
{
    auto research_token_sale = db_impl().find<research_token_sale_object, by_id>(id);
    FC_ASSERT(research_token_sale != nullptr, "Research token sale with id \"${1}\" must exist.", ("1", id));
}

const research_token_sale_object& dbs_research_token_sale::increase_tokens_amount(const research_token_sale_id_type &id,
                                                                                  const asset &amount)
{
    const research_token_sale_object& research_token_sale = get_by_id(id);
    db_impl().modify(research_token_sale, [&](research_token_sale_object& rts) { rts.total_amount += amount; });
    return research_token_sale;
}

const research_token_sale_object& dbs_research_token_sale::update_status(const research_token_sale_id_type &id,
                                                                         const research_token_sale_status& status)
{
    const research_token_sale_object& research_token_sale = get_by_id(id);
    db_impl().modify(research_token_sale, [&](research_token_sale_object& rts) { rts.status = status; });
    return research_token_sale;
}

const research_token_sale_contribution_object&
    dbs_research_token_sale::contribute(const research_token_sale_id_type &research_token_sale_id,
                                        const account_name_type &owner,
                                        const fc::time_point_sec contribution_time,
                                        const asset amount)
{
    const auto& new_research_token_sale_contribution
            = db_impl().create<research_token_sale_contribution_object>([&](research_token_sale_contribution_object& research_token_sale_contribution) {
                research_token_sale_contribution.research_token_sale_id = research_token_sale_id;
                research_token_sale_contribution.owner = owner;
                research_token_sale_contribution.contribution_time = contribution_time;
                research_token_sale_contribution.amount = amount;
            });

    return new_research_token_sale_contribution;
}

const research_token_sale_contribution_object&
    dbs_research_token_sale::get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type& id) const
{
    try {
        return db_impl().get<research_token_sale_contribution_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

dbs_research_token_sale::research_token_sale_contribution_refs_type
    dbs_research_token_sale::get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type& research_token_sale_id) const
{
    research_token_sale_contribution_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_sale_contribution_index>().indicies().get<by_research_token_sale_id>().equal_range(research_token_sale_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_token_sale_contribution_object&
    dbs_research_token_sale::get_research_token_sale_contribution_by_account_name_and_research_token_sale_id(const account_name_type& owner,
                                                                                                             const research_token_sale_id_type& research_token_sale_id) const
{
    try {
        return db_impl().get<research_token_sale_contribution_object, by_owner_and_research_token_sale_id>(
                boost::make_tuple(owner, research_token_sale_id));
    }
    FC_CAPTURE_AND_RETHROW((owner)(research_token_sale_id))
}

} // namespace chain
} // namespace deip

