#pragma once

#include <deip/app/plugin.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/investments_history/investments_history_objects.hpp>

#ifndef INVESTMENTS_HISTORY_PLUGIN_NAME
#define INVESTMENTS_HISTORY_PLUGIN_NAME "investments_history"
#endif

namespace deip {
namespace investments_history {

using namespace chain;
using app::application;

namespace detail {
class investments_history_plugin_impl;
}

class investments_history_plugin : public deip::app::plugin
{
public:
    investments_history_plugin(application* app);
    virtual ~investments_history_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;

    virtual void plugin_startup() override;

    friend class detail::investments_history_plugin_impl;

    std::unique_ptr<detail::investments_history_plugin_impl> my;
};
} // namespace investments_history
} // namespace deip