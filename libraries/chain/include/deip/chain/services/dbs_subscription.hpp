#pragma once

#include <fc/io/json.hpp>
#include "dbs_base_impl.hpp"
#include <deip/chain/schema/subscription_object.hpp>

namespace deip {
namespace chain {

class dbs_subscription : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_subscription() = delete;
protected:
    explicit dbs_subscription(database& db);

public:

    const subscription_object& create(const std::string& json_data, const research_group_id_type& research_group_id);

    const subscription_object& get(const subscription_id_type& id) const;

private:

    subscription_data_type get_data(const std::string& json_data)
    {
        subscription_data_type data = fc::json::from_string(json_data).as<subscription_data_type>();
        return data;
    }
};

} // namespace chain
} // namespace deip