#include <deip/chain/dbs_research_token_sale.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_research_token_sale::dbs_research_token_sale(database& db)
    : _base_type(db)
{
}

const dbs_research_token_sale_object&
dbs_research_token_sale::start_research_token_sale(const research_token_id_type& research_token_id,
                                                   const research_id_type& research_id,
                                                   const fc::time_point& start_time,
                                                   const fc::time_point& end_time,
                                                   const share_type total_amount,
                                                   const share_type balance_tokens,
                                                   const share_type soft_cap,
                                                   const share_type hard_cap)
{
    const dbs_research_token_sale_object& new_research_token_sale
        = db_impl().create<dbs_research_token_sale_object>([&](dbs_research_token_sale_object& research_token_sale) {
              research_token_sale.research_token_id = research_token_id;
              research_token_sale.research_id = research_id;
              research_token_sale.start_time = start_time;
              research_token_sale.end_time = end_time;
              research_token_sale.total_amount = total_amount;
              research_token_sale.balance_tokens = balance_tokens;
              research_token_sale.soft_cap = soft_cap;
              research_token_sale.hard_cap = hard_cap;
          });

    return new_research_token_sale;
}

} // namespace chain
} // namespace deip
