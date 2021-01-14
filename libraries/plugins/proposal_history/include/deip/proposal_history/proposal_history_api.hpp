#pragma once

#include <fc/api.hpp>
#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/proposal_history/proposal_history_api_objects.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace proposal_history {

using namespace std;
using namespace deip::chain;

namespace detail {
class proposal_history_api_impl;
}

class proposal_history_api
{
public:
    proposal_history_api(const deip::app::api_context& ctx);
    ~proposal_history_api();

    void on_api_startup();

    std::vector<proposal_state_api_obj> get_proposals_by_signer(const account_name_type& account) const;

    std::vector<proposal_state_api_obj> get_proposals_by_signers(const flat_set<account_name_type>& accounts) const;

    fc::optional<proposal_state_api_obj> get_proposal_state(const external_id_type& external_id) const;

    std::vector<proposal_state_api_obj> get_proposals_states(const flat_set<external_id_type>& external_ids) const;

private: 
    std::unique_ptr<detail::proposal_history_api_impl> _impl;

};
} // namespace proposal_history
} // namespace deip



FC_API(deip::proposal_history::proposal_history_api,
  (get_proposals_by_signer)
  (get_proposals_by_signers)
  (get_proposal_state)
  (get_proposals_states)
)

