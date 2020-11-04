#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research_token.hpp>
#include <deip/chain/services/dbs_research_token_sale.hpp>
#include <deip/chain/services/dbs_security_token.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/util/reward.hpp>

namespace deip {
namespace chain {

dbs_research_token_sale::dbs_research_token_sale(database& db)
    : _base_type(db)
{
}

const research_token_sale_object& dbs_research_token_sale::create_research_token_sale(const external_id_type& external_id,
                                                                                      const research_object& research,
                                                                                      const flat_map<external_id_type, security_token_amount_type>& security_tokens_on_sale,
                                                                                      const fc::time_point_sec& start_time,
                                                                                      const fc::time_point_sec& end_time,
                                                                                      const asset& soft_cap,
                                                                                      const asset& hard_cap)
{
    auto& dgp_service = db_impl().obtain_service<dbs_dynamic_global_properties>();

    FC_ASSERT(start_time >= db_impl().head_block_time(), "Start time must be >= current time");
    FC_ASSERT(end_time > start_time, "End time must be >= start time");

    const research_token_sale_object& new_research_token_sale
        = db_impl().create<research_token_sale_object>([&](research_token_sale_object& research_token_sale) {
              
              research_token_sale.external_id = external_id;
              for (const auto& security_token_on_sale : security_tokens_on_sale)
              {
                  research_token_sale.security_tokens_on_sale.insert(std::make_pair(security_token_on_sale.first, security_token_on_sale.second));
              }
              research_token_sale.research_id = research.id;
              research_token_sale.research_external_id = research.external_id;
              research_token_sale.start_time = start_time;
              research_token_sale.end_time = end_time;
              research_token_sale.total_amount = asset(0, soft_cap.symbol);
              research_token_sale.soft_cap = soft_cap;
              research_token_sale.hard_cap = hard_cap;
              research_token_sale.status = static_cast<uint16_t>(research_token_sale_status::inactive);
          });

    dgp_service.create_recent_entity(external_id);

    return new_research_token_sale;
}

const research_token_sale_object& dbs_research_token_sale::get_research_token_sale_by_id(const research_token_sale_id_type &id) const
{
    try {
        return db_impl().get<research_token_sale_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const research_token_sale_object& dbs_research_token_sale::get_research_token_sale(const external_id_type& external_id) const
{
    try
    {
        return db_impl().get<research_token_sale_object, by_external_id>(external_id);
    }
    FC_CAPTURE_AND_RETHROW((external_id))
}

const dbs_research_token_sale::research_token_sale_optional_ref_type
dbs_research_token_sale::get_research_token_sale_if_exists(const research_token_sale_id_type& id) const
{
    research_token_sale_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_token_sale_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const dbs_research_token_sale::research_token_sale_optional_ref_type
dbs_research_token_sale::get_research_token_sale_if_exists(const external_id_type& external_id) const
{
    research_token_sale_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_token_sale_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const dbs_research_token_sale::research_token_sale_refs_type dbs_research_token_sale::get_research_token_sales_by_research(const external_id_type& research_external_id) const
{
    research_token_sale_refs_type ret;

    auto it_pair = db_impl()
      .get_index<research_token_sale_index>()
      .indicies()
      .get<by_research>()
      .equal_range(research_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const dbs_research_token_sale::research_token_sale_refs_type dbs_research_token_sale::get_by_research_id(const research_id_type &research_id) const
{
    research_token_sale_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_sale_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_token_sale_object& dbs_research_token_sale::collect_funds(const research_token_sale_id_type& id,
                                                                         const asset& amount)
{
    const research_token_sale_object& research_token_sale = get_research_token_sale_by_id(id);
    db_impl().modify(research_token_sale, [&](research_token_sale_object& rts) { rts.total_amount += amount; });
    return research_token_sale;
}

const research_token_sale_object& dbs_research_token_sale::update_status(const research_token_sale_id_type &id,
                                                                         const research_token_sale_status& status)
{
    const research_token_sale_object& research_token_sale = get_research_token_sale_by_id(id);
    db_impl().modify(research_token_sale, [&](research_token_sale_object& rts) { rts.status = static_cast<uint16_t>(status); });
    return research_token_sale;
}

dbs_research_token_sale::research_token_sale_refs_type dbs_research_token_sale::get_by_research_id_and_status(
  const research_id_type& research_id,
  const research_token_sale_status& status) const
{
    research_token_sale_refs_type ret;

    auto it_pair = db_impl().get_index<research_token_sale_index>().indicies()
        .get<by_research_id_and_status>()
        .equal_range(boost::make_tuple(research_id, static_cast<uint16_t>(status)));

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const research_token_sale_contribution_object& dbs_research_token_sale::contribute(const research_token_sale_id_type& research_token_sale_id,
                                                                                   const account_name_type& owner,
                                                                                   const fc::time_point_sec& contribution_time,
                                                                                   const asset& amount)
{
    const auto& idx = db_impl()
      .get_index<research_token_sale_contribution_index>()
      .indicies()
      .get<by_owner_and_research_token_sale_id>();

    auto itr = idx.find(std::make_tuple(owner, research_token_sale_id));

    if (itr != idx.end())
    {
        const research_token_sale_contribution_object& existing_research_token_sale_contribution = *itr;
        db_impl().modify(existing_research_token_sale_contribution, [&](research_token_sale_contribution_object& rtsc_o) {
            rtsc_o.amount += amount;
        });

        return existing_research_token_sale_contribution;
    }
    else
    {
        const auto& research_token_sale = get_research_token_sale_by_id(research_token_sale_id);
        const research_token_sale_contribution_object& new_research_token_sale_contribution
            = db_impl().create<research_token_sale_contribution_object>(
                [&](research_token_sale_contribution_object& research_token_sale_contribution) {
                    research_token_sale_contribution.research_token_sale = research_token_sale.external_id;
                    research_token_sale_contribution.research_token_sale_id = research_token_sale.id;
                    research_token_sale_contribution.owner = owner;
                    research_token_sale_contribution.contribution_time = contribution_time;
                    research_token_sale_contribution.amount = amount;
                });

        return new_research_token_sale_contribution;
    }
}

const dbs_research_token_sale::research_token_sale_contribution_optional_ref_type
dbs_research_token_sale::get_research_token_sale_contribution_if_exists(const research_token_sale_contribution_id_type& id) const
{
    research_token_sale_contribution_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_token_sale_contribution_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const dbs_research_token_sale::research_token_sale_contribution_refs_type dbs_research_token_sale::get_research_token_sale_contributions_by_research_token_sale(const external_id_type& token_sale_external_id) const
{
    research_token_sale_contribution_refs_type ret;

    auto it_pair = db_impl()
      .get_index<research_token_sale_contribution_index>()
      .indicies()
      .get<by_research_token_sale>()
      .equal_range(token_sale_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const dbs_research_token_sale::research_token_sale_contribution_refs_type dbs_research_token_sale::get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type& research_token_sale_id) const
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
    dbs_research_token_sale::get_research_token_sale_contribution_by_contributor_and_research_token_sale_id(const account_name_type& owner,
                                                                                                            const research_token_sale_id_type& research_token_sale_id) const
{
    try {
        return db_impl().get<research_token_sale_contribution_object, by_owner_and_research_token_sale_id>(boost::make_tuple(owner, research_token_sale_id));
    }
    FC_CAPTURE_AND_RETHROW((owner)(research_token_sale_id))
}

const dbs_research_token_sale::research_token_sale_contribution_optional_ref_type
dbs_research_token_sale::get_research_token_sale_contribution_by_contributor_and_research_token_sale_id_if_exists(const account_name_type& owner,
                                                                                                                  const research_token_sale_id_type& research_token_sale_id) const
{
    research_token_sale_contribution_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_token_sale_contribution_index>()
      .indicies()
      .get<by_owner_and_research_token_sale_id>();

    auto itr = idx.find(std::make_tuple(owner, research_token_sale_id));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const dbs_research_token_sale::research_token_sale_contribution_refs_type dbs_research_token_sale::get_research_token_sale_contributions_by_contributor(const account_name_type& owner) const
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

void dbs_research_token_sale::finish_research_token_sale(const research_token_sale_id_type& research_token_sale_id)
{
    dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    dbs_research_group& research_group_service = db_impl().obtain_service<dbs_research_group>();
    dbs_research_token& research_token_service = db_impl().obtain_service<dbs_research_token>();
    dbs_security_token& security_token_service = db_impl().obtain_service<dbs_security_token>();
    dbs_account_balance& account_balance_service = db_impl().obtain_service<dbs_account_balance>();

    const auto& idx = db_impl()
      .get_index<research_token_sale_contribution_index>()
      .indicies()
      .get<by_research_token_sale_id>()
      .equal_range(research_token_sale_id);

    const auto& research_token_sale = get_research_token_sale_by_id(research_token_sale_id);
    const auto& research = research_service.get_research(research_token_sale.research_id);
    const auto& research_group = research_group_service.get_research_group(research.research_group_id);

    for (const auto& security_token_on_sale : research_token_sale.security_tokens_on_sale)
    {
        security_token_service.unfreeze_security_token(research.research_group, security_token_on_sale.first, security_token_on_sale.second);
    }

    auto itr = idx.first;
    const auto itr_end = idx.second;
    
    while (itr != itr_end)
    {
        for (const auto& security_token_on_sale : research_token_sale.security_tokens_on_sale)
        {
            const auto& security_token = security_token_service.get_security_token(security_token_on_sale.first);
            const auto& percent_share = percent(share_type(std::round((((double(itr->amount.amount.value) / double(research_token_sale.total_amount.amount.value)) * double(100)) * DEIP_1_PERCENT))));
            const auto& units = util::calculate_share(share_type(security_token_on_sale.second), percent_share);
            security_token_service.transfer_security_token(research.research_group, itr->owner, security_token_on_sale.first, uint32_t(units.value));
        }

        auto current = itr++;
        db_impl().remove(*current);
    }

    account_balance_service.adjust_account_balance(research_group.account, research_token_sale.total_amount);
}

void dbs_research_token_sale::refund_research_token_sale(const research_token_sale_id_type research_token_sale_id)
{
    dbs_account_balance& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    dbs_security_token& security_token_service = db_impl().obtain_service<dbs_security_token>();

    const auto& research_token_sale = get_research_token_sale_by_id(research_token_sale_id);
    const auto& research = research_service.get_research(research_token_sale.research_id);

    const auto& idx = db_impl()
      .get_index<research_token_sale_contribution_index>()
      .indicies()
      .get<by_research_token_sale_id>()
      .equal_range(research_token_sale_id);

    for (const auto& security_token_on_sale : research_token_sale.security_tokens_on_sale)
    {
        security_token_service.unfreeze_security_token(research.research_group, security_token_on_sale.first, security_token_on_sale.second);
    }

    auto itr = idx.first;
    const auto itr_end = idx.second;

    while (itr != itr_end)
    {
        account_balance_service.adjust_account_balance(itr->owner, itr->amount);

        auto current = itr++;
        db_impl().remove(*current);
    }
}


void dbs_research_token_sale::process_research_token_sales()
{
    const auto& idx = db_impl()
      .get_index<research_token_sale_index>()
      .indices()
      .get<by_end_time>();

    auto itr = idx.begin();
    const auto now = db_impl().head_block_time();

    while (itr != idx.end()) // TODO add index by status to decrease iteration time
    {
        if (itr->end_time <= now && itr->status == static_cast<uint16_t>(research_token_sale_status::active))
        {
            if (itr->total_amount < itr->soft_cap)
            {
                update_status(itr->id, research_token_sale_status::expired);
                refund_research_token_sale(itr->id);
            }
            else if (itr->total_amount >= itr->soft_cap)
            {
                update_status(itr->id, research_token_sale_status::finished);
                finish_research_token_sale(itr->id);
            }
        }
        else if (itr->end_time > now)
        {
            if (now >= itr->start_time && itr->status == static_cast<uint16_t>(research_token_sale_status::inactive))
            {
                update_status(itr->id, research_token_sale_status::active);
            }
        }

        itr++;
    }
}

} // namespace chain
} // namespace deip

