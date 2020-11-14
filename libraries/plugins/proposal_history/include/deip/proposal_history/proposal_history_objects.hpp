#pragma once

#ifndef PROPOSAL_HISTORY_SPACE_ID
#define PROPOSAL_HISTORY_SPACE_ID 17
#endif

using namespace deip::chain;
using namespace std;

namespace deip {
namespace proposal_history {

enum proposal_history_plugin_object_type
{
    proposal_state_object_type = (PROPOSAL_HISTORY_SPACE_ID << 8)
};

class proposal_state_object;

typedef oid<proposal_state_object> proposal_state_id_type;

} // namespace proposal_history
}

FC_REFLECT_ENUM(deip::proposal_history::proposal_history_plugin_object_type,
    (proposal_state_object_type)
)