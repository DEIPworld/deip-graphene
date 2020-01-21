#pragma once

#include <deip/app/plugin.hpp>
#include <deip/chain/database/database.hpp>

#ifndef CR_HISTORY_PLUGIN_NAME
#define CR_HISTORY_PLUGIN_NAME "research_content_references_history"
#endif

namespace deip {
namespace research_content_references_history {

using namespace chain;
using app::application;

namespace detail {
class research_content_reference_history_plugin_impl;
}

class research_content_reference_history_plugin : public deip::app::plugin
{
public:
    research_content_reference_history_plugin(application* app);
    virtual ~research_content_reference_history_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;

    virtual void plugin_startup() override;

    friend class detail::research_content_reference_history_plugin_impl;

    std::unique_ptr<detail::research_content_reference_history_plugin_impl> my;
};
}
} // deip::research_content_references_history