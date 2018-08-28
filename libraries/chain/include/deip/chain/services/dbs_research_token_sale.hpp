#pragma once

#include "dbs_base_impl.hpp"

#include <vector>
#include <set>
#include <functional>

#include "../schema/deip_object_types.hpp"
#include "../schema/research_token_sale_object.hpp"

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

    const research_token_sale_object& start(const research_id_type &research_id,
                                            const fc::time_point_sec start_time,
                                            const fc::time_point_sec end_time,
                                            const share_type& balance_tokens,
                                            const asset soft_cap,
                                            const asset hard_cap);

    research_token_sale_refs_type get_all() const;

    const research_token_sale_object& get_by_id(const research_token_sale_id_type &id) const;

    const research_token_sale_object& get_by_research_id(const research_id_type &research_id) const;

    research_token_sale_refs_type get_by_end_time(const fc::time_point_sec &end_time) const;

    void check_research_token_sale_existence(const research_token_sale_id_type& id) const;

    const research_token_sale_object& increase_tokens_amount(const research_token_sale_id_type &id,
                                                             const asset &amount);

    const research_token_sale_object& update_status(const research_token_sale_id_type &id,
                                                    const research_token_sale_status& status);

    //research_token_sale_contribution

    using research_token_sale_contribution_refs_type = std::vector<std::reference_wrapper<const research_token_sale_contribution_object>>;

    const research_token_sale_contribution_object& contribute(const research_token_sale_id_type &research_token_sale_id,
                                                              const account_name_type &owner,
                                                              const fc::time_point_sec contribution_time,
                                                              const asset amount);

    const research_token_sale_contribution_object& get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type& id) const;

    research_token_sale_contribution_refs_type
        get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type& research_token_sale_id) const;

    const research_token_sale_contribution_object& get_research_token_sale_contribution_by_account_name_and_research_token_sale_id
            (const account_name_type& owner, const research_token_sale_id_type& research_token_sale_id) const;
};

} // namespace chain
} // namespace deip
