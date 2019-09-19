#pragma once

#include <fc/io/json.hpp>
#include "dbs_base_impl.hpp"
#include <deip/chain/schema/subscription_object.hpp>
#include <deip/chain/schema/subscription_data_types.hpp>

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

    void set_new_billing_date(const subscription_object& subscription);

    void check_subscription_existence(const subscription_id_type& subscription_id) const;

    void adjust_additional_limits(const subscription_object& subscription, const std::string& json_data);

private:

    template <typename DataType> DataType get_data(const std::string& json_data)
    {
        auto data = fc::json::from_string(json_data).as<DataType>();
        return data;
    }
};

} // namespace chain
} // namespace deip