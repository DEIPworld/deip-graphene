#pragma once

#include <fc/api.hpp>
#include <deip/ip_protection_history/applied_ip_protection_operation.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace ip_protection_history {

namespace detail {
class ip_protection_history_api_impl;
}

class ip_protection_history_api
{
public:
    ip_protection_history_api(const deip::app::api_context& ctx);
    ~ip_protection_history_api();

    void on_api_startup();

    std::vector<applied_ip_protection_operation>
    get_content_history(const research_id_type& research_id, 
                        const std::string& content_hash) const;


private:
    std::unique_ptr<detail::ip_protection_history_api_impl> _impl;
};
} // namespace ip_protection_history
} // namespace deip

FC_API(deip::ip_protection_history::ip_protection_history_api,
       (get_content_history))
