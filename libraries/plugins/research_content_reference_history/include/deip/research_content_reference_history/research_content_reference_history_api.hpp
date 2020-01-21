#pragma once

#include <fc/api.hpp>
#include <deip/research_content_reference_history/applied_research_content_reference_operation.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace research_content_references_history {

namespace detail {
class research_content_reference_history_api_impl;
}

class research_content_reference_history_api
{
public:
    research_content_reference_history_api(const deip::app::api_context& ctx);
    ~research_content_reference_history_api();

    void on_api_startup();

    std::vector<applied_research_content_reference_operation>
    get_content_references(const research_content_id_type& research_content_id) const;

    std::vector<applied_research_content_reference_operation>
    get_contents_refer_to_content(const research_content_id_type& research_content_id) const;

private:
    std::unique_ptr<detail::research_content_reference_history_api_impl> _impl;
};
} // namespace research_content_references_history
} // namespace deip

FC_API(deip::research_content_references_history::research_content_reference_history_api,
       (get_content_references)(get_contents_refer_to_content))