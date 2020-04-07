#pragma once

#include <fc/api.hpp>
#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/fo_history/fo_history_api_objects.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace fo_history {

using namespace std;
using namespace deip::chain;

namespace detail {
class fo_history_api_impl;
}

class fo_history_api
{
public:
    fo_history_api(const deip::app::api_context& ctx);
    ~fo_history_api();

    void on_api_startup();
    
    std::vector<withdrawal_request_history_api_obj> get_withdrawal_requests_history_by_award_number(const string& award_number) const;

    std::vector<withdrawal_request_history_api_obj> get_withdrawal_request_history_by_award_and_payment_number(const string& award_number,
                                                                                                               const string& payment_number) const;

    std::vector<withdrawal_request_history_api_obj> get_withdrawal_requests_history_by_award_and_subaward_number(const string& award_number,
                                                                                                                 const string& subaward_number) const;
    
private:
    std::unique_ptr<detail::fo_history_api_impl> _impl;
};
} // namespace fo_history
} // namespace deip

FC_API(deip::fo_history::fo_history_api,

    (get_withdrawal_requests_history_by_award_number)
    (get_withdrawal_request_history_by_award_and_payment_number)
    (get_withdrawal_requests_history_by_award_and_subaward_number)

)