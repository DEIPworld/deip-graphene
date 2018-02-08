#include <deip/chain/dbs_research_token_sale.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_research_token_sale::dbs_research_token_sale(database& db)
    : _base_type(db)
{
}

const research_token_sale_object& dbs_research_token_sale::start_research_token_sale(const research_id_type& research_id,
                                                                                     const fc::time_point start_time,
                                                                                     const fc::time_point end_time,
                                                                                     const deip::chain::share_type balance_tokens,
                                                                                     const deip::chain::share_type soft_cap,
                                                                                     const deip::chain::share_type hard_cap)
{
    const research_token_sale_object& new_research_token_sale
        = db_impl().create<research_token_sale_object>([&](research_token_sale_object& research_token_sale) {
              research_token_sale.research_id = research_id;
              research_token_sale.start_time = start_time;
              research_token_sale.end_time = end_time;
              research_token_sale.total_amount = 0;
              research_token_sale.balance_tokens = balance_tokens;
              research_token_sale.soft_cap = soft_cap;
              research_token_sale.hard_cap = hard_cap;
          });

    return new_research_token_sale;
}

const research_token_sale_object&
dbs_research_token_sale::get_research_token_sale_by_id(const research_token_sale_id_type& id) const
{
    return db_impl().get<research_token_sale_object, by_id>(id);
}

const research_token_sale_object&
dbs_research_token_sale::get_research_token_sale_by_research_id(const research_id_type& research_id) const
{
    return db_impl().get<research_token_sale_object, by_research_id>(research_id);
}

dbs_research_token_sale::research_token_sale_refs_type
dbs_research_token_sale::get_research_token_sale_by_end_time(const fc::time_point& end_time) const
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

} // namespace chain
} // namespace deip

