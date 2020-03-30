#pragma once

#include <deip/app/plugin.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/eci_history/eci_history_objects.hpp>

#ifndef ECI_HISTORY_PLUGIN_NAME
#define ECI_HISTORY_PLUGIN_NAME "eci_history"
#endif

namespace deip {
namespace eci_history {

using namespace chain;
using app::application;

namespace detail {
class eci_history_plugin_impl;
}

class eci_history_plugin : public deip::app::plugin
{
public:
    eci_history_plugin(application* app);
    virtual ~eci_history_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;

    virtual void plugin_startup() override;

    friend class detail::eci_history_plugin_impl;

    std::unique_ptr<detail::eci_history_plugin_impl> my;
};
} // namespace eci_history
} // namespace deip