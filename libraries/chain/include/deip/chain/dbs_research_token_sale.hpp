#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/research_group_object.hpp>

namespace deip {
namespace chain {

class proposal_object;

class dbs_research_token_sale : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_research_token_sale() = delete;

protected:
    explicit dbs_research_token_sale(database& db);

public:
    const dbs_research_token_sale_object&
    dbs_research_token_sale::start_research_token_sale(const research_token_id_type& research_token_id,
                                                       const research_id_type& research_id,
                                                       const fc::time_point& start_time,
                                                       const fc::time_point& end_time,
                                                       const share_type balance_tokens,
                                                       const share_type soft_cap,
                                                       const share_type hard_cap);
};

} // namespace chain
} // namespace deip
