#pragma once

#include <deip/app/application.hpp>

#include <deip/account_by_key/account_by_key_objects.hpp>

#include <fc/api.hpp>

namespace deip {
namespace account_by_key {


namespace detail {
class account_by_key_api_impl;
}

struct key_reference
{
    account_name_type account;
    public_key_type key;
    time_point_sec deactivation_time;
};

struct team_reference
{
    account_name_type account;
    account_name_type team;
    time_point_sec deactivation_time;
};

class account_by_key_api
{
public:
    account_by_key_api(const app::api_context& ctx);

    void on_api_startup();

    vector<vector<key_reference>> get_key_references(const vector<public_key_type>& keys, const bool& full_history = false) const;

    vector<vector<key_reference>> get_account_key_references(const vector<account_name_type>& accounts, const bool& full_history = false) const;

    vector<vector<team_reference>> get_team_references(const vector<account_name_type>& teams, const bool& full_history = false) const;

    vector<vector<team_reference>> get_team_member_references(const vector<account_name_type>& members, const bool& full_history = false) const;

private:
    std::shared_ptr<detail::account_by_key_api_impl> my;
};

}
} // deip::account_by_key


FC_REFLECT( deip::account_by_key::key_reference,
            (account)
            (key)
            (deactivation_time)
)

FC_REFLECT( deip::account_by_key::team_reference,
            (account)
            (team)
            (deactivation_time)
)

FC_API(deip::account_by_key::account_by_key_api, 
  (get_key_references)
  (get_account_key_references)
  (get_team_references)
  (get_team_member_references)
)
