
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace deip {
namespace app {

class abstract_plugin;
class application;
}
}

namespace deip {
namespace plugin {

void initialize_plugin_factories();
std::shared_ptr<deip::app::abstract_plugin> create_plugin(const std::string& name, deip::app::application* app);
std::vector<std::string> get_available_plugins();
}
}
