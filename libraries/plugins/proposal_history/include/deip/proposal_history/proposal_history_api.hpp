#pragma once

#include <fc/api.hpp>
#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/proposal_history/proposal_history_api_objects.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace proposal_history {

using namespace std;
using namespace deip::chain;

namespace detail {
class proposal_history_api_impl;
}

class proposal_history_api
{
public:
    proposal_history_api(const deip::app::api_context& ctx);
    ~proposal_history_api();

    void on_api_startup();

    // std::vector<account_revenue_income_history_api_obj> get_account_revenue_history_by_security_token(const account_name_type& account,
    //                                                                                                   const string& security_token_symbol,
    //                                                                                                   const account_revenue_income_history_id_type& cursor,
    //                                                                                                   const fc::optional<uint16_t> step,
    //                                                                                                   const fc::optional<string> target_asset_symbol) const;

private: 
    std::unique_ptr<detail::proposal_history_api_impl> _impl;

};
} // namespace proposal_history
} // namespace deip



FC_API(deip::proposal_history::proposal_history_api,
  // (get_account_revenue_history_by_security_token)
)

