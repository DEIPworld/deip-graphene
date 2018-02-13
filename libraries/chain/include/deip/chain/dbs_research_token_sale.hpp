#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/research_token_sale_object.hpp>

namespace deip {
namespace chain {
class dbs_research_token_sale : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_research_token_sale() = delete;

protected:
    explicit dbs_research_token_sale(database& db);

public:
    using research_token_sale_refs_type = std::vector<std::reference_wrapper<const research_token_sale_object>>;

    const research_token_sale_object& start_research_token_sale(const research_id_type& research_id,
                                                                const fc::time_point_sec start_time,
                                                                const fc::time_point_sec end_time,
                                                                const deip::chain::share_type balance_tokens,
                                                                const deip::chain::share_type soft_cap,
                                                                const deip::chain::share_type hard_cap);

    research_token_sale_refs_type get_all_research_token_sales() const;

    const research_token_sale_object& get_research_token_sale_by_id(const research_token_sale_id_type& id) const;

    const research_token_sale_object& get_research_token_sale_by_research_id(const research_id_type& research_id) const;
    research_token_sale_refs_type get_research_token_sale_by_end_time(const fc::time_point_sec& end_time) const;

    void check_research_token_sale_existence(const research_token_sale_id_type& id) const;

    const research_token_sale_object& increase_research_token_sale_tokens_amount(const research_token_sale_id_type& id, const share_type amount);

    //research_token_sale_contribution

    using research_token_sale_contribution_refs_type = std::vector<std::reference_wrapper<const research_token_sale_contribution_object>>;

    const research_token_sale_contribution_object& create_research_token_sale_contribution(const research_token_sale_id_type& research_token_sale_id,
                                                                                           const account_name_type& owner,
                                                                                           const fc::time_point_sec contribution_time,
                                                                                           const deip::chain::share_type amount);

    const research_token_sale_contribution_object& get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type& id) const;

    research_token_sale_contribution_refs_type
        get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type& research_token_sale_id) const;

    research_token_sale_contribution_refs_type get_research_token_sale_contributions_by_account_name(const account_name_type& owner) const;
};

} // namespace chain
} // namespace deip
