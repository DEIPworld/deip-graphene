#pragma once

#ifndef ECI_HISTORY_SPACE_ID
#define ECI_HISTORY_SPACE_ID 13
#endif

using namespace deip::chain;
using namespace std;

namespace deip {
namespace eci_history {

enum eci_history_plugin_object_type
{
    research_content_eci_history_object_type = (ECI_HISTORY_SPACE_ID << 8),
    research_eci_history_object_type,
    account_eci_history_object_type
};

class account_eci_history_object;
class research_content_eci_history_object;
class research_eci_history_object;

typedef oid<research_content_eci_history_object> research_content_eci_history_id_type;
typedef oid<research_eci_history_object> research_eci_history_id_type;
typedef oid<account_eci_history_object> account_eci_history_id_type;

}
}

FC_REFLECT_ENUM(deip::eci_history::eci_history_plugin_object_type,
  (research_content_eci_history_object_type)
  (research_eci_history_object_type)
  (account_eci_history_object_type)
)