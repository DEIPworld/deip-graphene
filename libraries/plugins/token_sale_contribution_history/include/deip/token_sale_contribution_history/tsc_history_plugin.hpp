#pragma once

#include <deip/app/plugin.hpp>
#include <deip/chain/database/database.hpp>

#ifndef TSC_HISTORY_PLUGIN_NAME
#define TSC_HISTORY_PLUGIN_NAME "tsc_history"
#endif

namespace deip {
namespace tsc_history {

using namespace chain;
using app::application;

namespace detail {
class tsc_history_plugin_impl;
}

/**
 *  This plugin is designed to track a range of operations by account so that one node
 *  doesn't need to hold the full operation history in memory.
 */
class tsc_history_plugin : public deip::app::plugin
{
public:
    tsc_history_plugin(application* app);
    virtual ~tsc_history_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;

    virtual void plugin_startup() override;

    friend class detail::tsc_history_plugin_impl;

    std::unique_ptr<detail::tsc_history_plugin_impl> my;
};
}
} // deip::tsc_history