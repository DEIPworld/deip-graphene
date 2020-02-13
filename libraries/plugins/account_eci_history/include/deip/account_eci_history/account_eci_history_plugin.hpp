#pragma once

#include <deip/app/plugin.hpp>
#include <deip/chain/database/database.hpp>

#ifndef ACCOUNT_ECI_HISTORY_PLUGIN_NAME
#define ACCOUNT_ECI_HISTORY_PLUGIN_NAME "account_eci_history"
#endif

namespace deip {
namespace account_eci_history {

using namespace chain;
using app::application;

namespace detail {
class account_eci_history_plugin_impl;
}

class account_eci_history_plugin : public deip::app::plugin
{
public:
    account_eci_history_plugin(application* app);
    virtual ~account_eci_history_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;

    virtual void plugin_startup() override;

    friend class detail::account_eci_history_plugin_impl;

    std::unique_ptr<detail::account_eci_history_plugin_impl> my;
};
}
} // deip::account_eci_history
