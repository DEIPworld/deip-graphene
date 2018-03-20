#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/research_group_join_request_object.hpp>

#include <fc/shared_string.hpp>
#include <fc/fixed_string.hpp>

namespace deip {
namespace chain {

class dbs_research_group_join_request : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_research_group_join_request() = delete;
protected:
    explicit dbs_research_group_join_request(database& db);

public:

    using research_group_join_request_refs_type = std::vector<std::reference_wrapper<const research_group_join_request_object>>;

    const research_group_join_request_object& create(const account_name_type& account_name,
                                                     const research_group_id_type& research_group_id,
                                                     const std::string motivation_letter);

    const research_group_join_request_object& get(const research_group_join_request_id_type& research_group_invite_id);

    const research_group_join_request_object& get_research_group_join_request_by_account_name_and_research_group_id(const account_name_type& account_name,
                                                                                                                    const research_group_id_type& research_group_id);

    void check_research_group_join_request_existence(const research_group_join_request_id_type& research_group_join_request_id);

    research_group_join_request_refs_type get_research_group_join_requests_by_account_name(const account_name_type& account_name);

    research_group_join_request_refs_type get_research_group_join_requests_by_research_group_id(const research_group_id_type& research_group_id);

};

} // namespace chain
} // namespace deip
