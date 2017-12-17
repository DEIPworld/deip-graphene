#pragma once

#include <deip/account_statistics/account_statistics_plugin.hpp>

#include <fc/api.hpp>

namespace deip {
namespace app {
struct api_context;
}
}

namespace deip {
namespace account_statistics {

namespace detail {
class account_statistics_api_impl;
}

class account_statistics_api
{
public:
    account_statistics_api(const deip::app::api_context& ctx);

    void on_api_startup();

private:
    std::shared_ptr<detail::account_statistics_api_impl> _my;
};
}
} // deip::account_statistics

FC_API(deip::account_statistics::account_statistics_api, )