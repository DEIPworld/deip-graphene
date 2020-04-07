#include <deip/fo_history/fo_history_plugin.hpp>
#include <deip/fo_history/fo_history_api.hpp>

#include <deip/fo_history/withdrawal_request_history_object.hpp>

#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <map>

namespace deip {
namespace fo_history {

namespace detail {

class fo_history_api_impl
{
public:
    deip::app::application& _app;

public:
    fo_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    std::vector<withdrawal_request_history_api_obj> get_withdrawal_requests_history_by_award_number(const string& award_number) const
    {
        std::vector<withdrawal_request_history_api_obj> result;

        const auto db = _app.chain_database();
        const auto& idx = db->get_index<withdrawal_request_history_index>().indices().get<by_award_number>();

        auto it_pair = idx.equal_range(award_number);

        auto it = it_pair.first;
        const auto it_end = it_pair.second;
        while (it != it_end)
        {
            result.push_back(withdrawal_request_history_api_obj(*it));
            ++it;
        }

        return result;
    }

    std::vector<withdrawal_request_history_api_obj> get_withdrawal_request_history_by_award_and_payment_number(const string& award_number,
                                                                                                               const string& payment_number) const
    {
        std::vector<withdrawal_request_history_api_obj> result;

        const auto db = _app.chain_database();
        const auto& idx = db->get_index<withdrawal_request_history_index>().indices().get<by_award_and_payment_number>();

        auto it_pair = idx.equal_range(std::make_tuple(award_number, payment_number));

        auto it = it_pair.first;
        const auto it_end = it_pair.second;
        while (it != it_end)
        {
            result.push_back(withdrawal_request_history_api_obj(*it));
            ++it;
        }

        return result;
    }

    std::vector<withdrawal_request_history_api_obj> get_withdrawal_requests_history_by_award_and_subaward_number(const string& award_number,
                                                                                                              const string& subaward_number) const
    {
        std::vector<withdrawal_request_history_api_obj> result;

        const auto db = _app.chain_database();
        const auto& idx = db->get_index<withdrawal_request_history_index>().indices().get<by_award_and_subaward_number>();

        auto it_pair = idx.equal_range(std::make_tuple(award_number, subaward_number));

        auto it = it_pair.first;
        const auto it_end = it_pair.second;
        while (it != it_end)
        {
            result.push_back(withdrawal_request_history_api_obj(*it));
            ++it;
        }

        return result;
    }



};
} // namespace detail

fo_history_api::fo_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::fo_history_api_impl(ctx.app))
{
}

fo_history_api::~fo_history_api()
{
}

void fo_history_api::on_api_startup()
{
}

std::vector<withdrawal_request_history_api_obj>
fo_history_api::get_withdrawal_requests_history_by_award_number(const string& award_number) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_withdrawal_requests_history_by_award_number(award_number);
    });
}

std::vector<withdrawal_request_history_api_obj>
fo_history_api::get_withdrawal_request_history_by_award_and_payment_number(const string& award_number,
                                                                           const string& payment_number) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_withdrawal_request_history_by_award_and_payment_number(award_number, payment_number);
    });
}

std::vector<withdrawal_request_history_api_obj>
fo_history_api::get_withdrawal_requests_history_by_award_and_subaward_number(const string& award_number,
                                                                             const string& subaward_number) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_withdrawal_requests_history_by_award_and_subaward_number(award_number, subaward_number);
    });
}



} // namespace fo_history
} // namespace deip