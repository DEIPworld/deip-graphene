#pragma once

#include <boost/multi_index/composite_key.hpp>

using namespace deip::chain;
using namespace std;

namespace deip {
namespace eci_history {

using chainbase::allocator;

class account_eci_history_object : public object<account_eci_history_object_type, account_eci_history_object>
{
public:
    template <typename Constructor, typename Allocator>
    account_eci_history_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    account_eci_history_object()
    {
    }

    account_eci_history_id_type id;

    account_name_type account;
    discipline_id_type discipline_id;

    share_type eci;
    share_type delta;

    uint16_t contribution_type;
    int64_t contribution_id;

    fc::time_point_sec timestamp;
};

struct by_account;
struct by_account_and_discipline;

typedef chainbase::shared_multi_index_container<account_eci_history_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
         member<
          account_eci_history_object,
          account_eci_history_id_type,
          &account_eci_history_object::id
        >
    >,
    ordered_non_unique<
      tag<by_account>,
         member<
          account_eci_history_object,
          account_name_type,
          &account_eci_history_object::account
        >
    >,
    ordered_non_unique<
      tag<by_account_and_discipline>,
        composite_key<account_eci_history_object,
          member<
            account_eci_history_object,
            account_name_type,
            &account_eci_history_object::account
          >,
          member<
            account_eci_history_object,
            discipline_id_type,
            &account_eci_history_object::discipline_id
          >
        >
    >>
    // , allocator<account_eci_history_object>
    >
    account_eci_history_index;

} // namespace eci_history
} // namespace deip

FC_REFLECT(deip::eci_history::account_eci_history_object,
  (id)
  (account)
  (discipline_id)
  (eci)
  (delta)
  (contribution_type)
  (contribution_id)
  (timestamp)
)

CHAINBASE_SET_INDEX_TYPE(deip::eci_history::account_eci_history_object, deip::eci_history::account_eci_history_index)
