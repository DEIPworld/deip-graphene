#pragma once

#include <deip/app/plugin.hpp>
#include <deip/chain/database/database.hpp>

#ifndef IP_PROTECTION_HISTORY_PLUGIN_NAME
#define IP_PROTECTION_HISTORY_PLUGIN_NAME "ip_protection_history"
#endif

namespace deip {
namespace ip_protection_history {

using namespace chain;
using app::application;

namespace detail {
class ip_protection_history_plugin_impl;
}

/**
 *  This plugin is designed to track a range of operations by account so that one node
 *  doesn't need to hold the full operation history in memory.
 */
class ip_protection_history_plugin : public deip::app::plugin
{
public:
    ip_protection_history_plugin(application* app);
    virtual ~ip_protection_history_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;

    virtual void plugin_startup() override;

    friend class detail::ip_protection_history_plugin_impl;

    std::unique_ptr<detail::ip_protection_history_plugin_impl> my;
};
}
} // deip::ip_protection_history
