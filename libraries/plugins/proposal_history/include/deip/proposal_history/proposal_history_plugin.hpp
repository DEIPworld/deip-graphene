#pragma once

#include <deip/app/plugin.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/proposal_history/proposal_history_objects.hpp>

#ifndef PROPOSAL_HISTORY_PLUGIN_NAME
#define PROPOSAL_HISTORY_PLUGIN_NAME "proposal_history"
#endif

namespace deip {
namespace proposal_history {

using namespace chain;
using app::application;

namespace detail {
class proposal_history_plugin_impl;
}

class proposal_history_plugin : public deip::app::plugin
{
public:
    proposal_history_plugin(application* app);
    virtual ~proposal_history_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;

    virtual void plugin_startup() override;

    friend class detail::proposal_history_plugin_impl;

    std::unique_ptr<detail::proposal_history_plugin_impl> my;    
};
} // namespace proposal_history
} // namespace deip