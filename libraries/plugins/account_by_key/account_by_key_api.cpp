#include <deip/account_by_key/account_by_key_api.hpp>
#include <deip/account_by_key/account_by_key_objects.hpp>

namespace deip {
namespace account_by_key {

namespace detail {

class account_by_key_api_impl
{
public:
    account_by_key_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    vector<vector<key_reference>> get_key_references(const vector<public_key_type>& keys, const bool& full_history) const;

    deip::app::application& _app;
};

vector<vector<key_reference>> account_by_key_api_impl::get_key_references(const vector<public_key_type>& keys, const bool& full_history) const
{
    vector<vector<key_reference>> final_result;
    final_result.reserve(keys.size());

    const auto& key_idx = _app.chain_database()->get_index<key_lookup_index>().indices().get<by_key>();

    for (auto& key : keys)
    {
        vector<key_reference> result;
        auto lookup_itr = key_idx.lower_bound(key);

        for (auto lookup_itr = key_idx.lower_bound(key); lookup_itr != key_idx.end() && lookup_itr->key == key; ++lookup_itr)
        {
            const key_lookup_object& key_lookup = *lookup_itr;

            key_reference key_ref;
            key_ref.account = key_lookup.account;
            key_ref.key = key_lookup.key;
            key_ref.deactivation_time = key_lookup.deactivation_time;

            if (full_history)
            {
                result.push_back(key_ref);
            }
            else
            {
                if (key_lookup.deactivation_time == fc::time_point_sec::maximum())
                {
                    result.push_back(key_ref);
                }
            }
        }

        final_result.emplace_back(std::move(result));
    }

    return final_result;
}

} // detail

account_by_key_api::account_by_key_api(const deip::app::api_context& ctx)
{
    my = std::make_shared<detail::account_by_key_api_impl>(ctx.app);
}

void account_by_key_api::on_api_startup()
{
}

vector<vector<key_reference>> account_by_key_api::get_key_references(const vector<public_key_type>& keys, const bool& full_history) const
{
    return my->_app.chain_database()->with_read_lock([&]() { return my->get_key_references(keys, full_history); });
}
}
} // deip::account_by_key
