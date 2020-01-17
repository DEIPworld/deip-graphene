#pragma once

#include <fc/api.hpp>
#include <deip/content_references_history/applied_cr_operation.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace cr_history {

namespace detail {
class cr_history_api_impl;
}

class cr_history_api
{
public:
    cr_history_api(const deip::app::api_context& ctx);
    ~cr_history_api();

    void on_api_startup();

    std::vector<applied_cr_operation>
    get_content_references(const research_content_id_type& research_content_id) const;

    std::vector<applied_cr_operation>
    get_contents_refer_to_content(const research_content_id_type& research_content_id) const;


private:
    std::unique_ptr<detail::cr_history_api_impl> _impl;
};
} // namespace cr_history
} // namespace deip

FC_API(deip::cr_history::cr_history_api,
       (get_content_references)(get_contents_refer_to_content))