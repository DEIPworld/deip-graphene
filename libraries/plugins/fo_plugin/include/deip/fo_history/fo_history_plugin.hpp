#pragma once

#include <deip/app/plugin.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/fo_history/fo_history_objects.hpp>

#ifndef FO_HISTORY_PLUGIN_NAME
#define FO_HISTORY_PLUGIN_NAME "fo_history"
#endif

namespace deip {
namespace fo_history {

using namespace chain;
using app::application;

namespace detail {
class fo_history_plugin_impl;
}

class fo_history_plugin : public deip::app::plugin
{
public:
    fo_history_plugin(application* app);
    virtual ~fo_history_plugin();

    std::string plugin_name() const override;

    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;

    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;

    virtual void plugin_startup() override;

    friend class detail::fo_history_plugin_impl;

    std::unique_ptr<detail::fo_history_plugin_impl> my;
};
} // namespace fo_history
} // namespace deip