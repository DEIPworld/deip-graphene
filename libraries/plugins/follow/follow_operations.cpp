#include <deip/follow/follow_operations.hpp>

#include <deip/protocol/operation_util_impl.hpp>

namespace deip {
namespace follow {

void follow_operation::validate() const
{
    FC_ASSERT(follower != following, "You cannot follow yourself");
}

void reblog_operation::validate() const
{
    FC_ASSERT(account != author, "You cannot reblog your own content");
}
}
} // deip::follow

DEFINE_OPERATION_TYPE(deip::follow::follow_plugin_operation)
