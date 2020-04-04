#pragma once

#ifndef FO_HISTORY_SPACE_ID
#define FO_HISTORY_SPACE_ID 16
#endif

using namespace deip::chain;
using namespace std;

namespace deip {
namespace fo_history {

enum fo_history_plugin_object_type
{
    withdrawal_request_history_object_type = (FO_HISTORY_SPACE_ID << 8)
};

class withdrawal_request_history_object;

typedef oid<withdrawal_request_history_object> withdrawal_request_history_id_type;

}
}

FC_REFLECT_ENUM(deip::fo_history::fo_history_plugin_object_type,
    (withdrawal_request_history_object_type)
)