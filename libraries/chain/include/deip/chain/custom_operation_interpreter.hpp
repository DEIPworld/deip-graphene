
#pragma once

#include <memory>

#include <deip/protocol/deip_operations.hpp>

namespace graphene {
namespace schema {
struct abstract_schema;
}
}

namespace deip {
namespace protocol {
struct custom_json_operation;
}
}

namespace deip {
namespace chain {

class custom_operation_interpreter
{
public:
    virtual void apply(const protocol::custom_json_operation& op) = 0;
    virtual void apply(const protocol::custom_binary_operation& op) = 0;
    virtual std::shared_ptr<graphene::schema::abstract_schema> get_operation_schema() = 0;
};
}
} // deip::chain
