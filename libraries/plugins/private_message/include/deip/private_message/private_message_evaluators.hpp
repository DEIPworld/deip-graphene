#pragma once

#include <deip/chain/evaluator.hpp>

#include <deip/private_message/private_message_operations.hpp>
#include <deip/private_message/private_message_plugin.hpp>

namespace deip {
namespace private_message {

DEFINE_PLUGIN_EVALUATOR(
    private_message_plugin, deip::private_message::private_message_plugin_operation, private_message)
}
}
