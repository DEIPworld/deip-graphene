#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/research_token_sale_contribution_object.hpp>

namespace deip {
namespace chain {
class dbs_research_token_sale_contribution : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_research_token_sale_contribution() = delete;

protected:
    explicit dbs_research_token_sale_contribution(database& db);

public:
    using research_token_sale_contribution_refs_type = std::vector<std::reference_wrapper<const research_token_sale_contribution_object>>;

    const research_token_sale_contribution_object& create_research_token_sale_contribution(const research_token_sale_id_type& research_token_sale_id,
                                                                                           const account_name_type& owner,
                                                                                           const fc::time_point_sec contribution_time,
                                                                                           const deip::chain::share_type amount);

    const research_token_sale_contribution_object& get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type& id) const;

    research_token_sale_contribution_refs_type
        get_research_token_sale_contribution_by_research_token_sale_id(const research_token_sale_id_type& research_token_sale_id) const;

    research_token_sale_contribution_refs_type get_research_token_sale_contribution_by_account_name(const account_name_type& owner) const;
};

} // namespace chain
} // namespace deip
