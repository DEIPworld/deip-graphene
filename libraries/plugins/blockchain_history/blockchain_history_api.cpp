#include <deip/blockchain_history/blockchain_history_api.hpp>
#include <deip/app/application.hpp>
#include <deip/blockchain_history/operation_objects.hpp>

#include <fc/static_variant.hpp>

#include <boost/lambda/lambda.hpp>

namespace deip {
namespace blockchain_history {

namespace detail {

using result_type = std::map<uint32_t, applied_operation>;

class blockchain_history_api_impl
{
public:
    deip::app::application& _app;
    std::shared_ptr<chain::database> _db;

private:
    template <typename ObjectType> applied_operation get_filtered_operation(const ObjectType& obj) const
    {
        return _db->get(obj.op);
    }

    applied_operation get_operation(const filtered_not_virt_operations_history_object& obj) const
    {
        return get_filtered_operation(obj);
    }

    applied_operation get_operation(const filtered_virt_operations_history_object& obj) const
    {
        return get_filtered_operation(obj);
    }

    applied_operation get_operation(const filtered_market_operations_history_object& obj) const
    {
        return get_filtered_operation(obj);
    }
    
    applied_operation get_operation(const operation_object& obj) const
    {
        applied_operation temp;
        temp = obj;
        return temp;
    }

public:
    blockchain_history_api_impl(deip::app::application& app)
        : _app(app)
        , _db(_app.chain_database())
    {
    }

    using result_type = std::map<uint32_t, applied_operation>;
     template <typename IndexType> result_type get_ops_history(uint32_t from_op, uint32_t limit) const
    {
        using namespace deip::chain;

        static const uint32_t max_history_depth = 100;

        FC_ASSERT(limit > 0, "Limit must be greater than zero");
        FC_ASSERT(limit <= max_history_depth, "Limit of ${l} is greater than maxmimum allowed ${2}",
                  ("l", limit)("2", max_history_depth));
        FC_ASSERT(from_op >= limit, "From must be greater than limit");

        return _db->with_read_lock([&]() {

            result_type result;

            const auto& idx = _db->get_index<IndexType>().indices().template get<by_id>();
            if (idx.empty())
                return result;

            // move to last operation object
            auto itr = idx.lower_bound(from_op);
            if (itr == idx.end())
                --itr;
            auto start = (int64_t(itr->id._id) - limit);
            auto end = itr->id._id;
            auto range = idx.range(start < boost::lambda::_1, boost::lambda::_1 <= end);

            for (auto it = range.first; it != range.second; ++it)
            {
                auto id = it->id;
                FC_ASSERT(id._id >= 0, "Invalid operation_object id");
                result[(uint32_t)id._id] = get_operation(*it);
            }
            return result;
        });
    }
};

bool operation_filter(operation& op, const applied_operation_type& opt)
{
    switch (opt)
    {
    case applied_operation_type::market:
        return is_market_operation(op);
    case applied_operation_type::virt:
        return is_virtual_operation(op);
    case applied_operation_type::not_virt:
        return !is_virtual_operation(op);
    default:;
    }
    return true;
}

} // namespace detail

blockchain_history_api::blockchain_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::blockchain_history_api_impl(ctx.app))
{
}

blockchain_history_api::~blockchain_history_api()
{
}

void blockchain_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation>
blockchain_history_api::get_ops_history(uint32_t from_op, uint32_t limit, const applied_operation_type& opt) const
{
    switch (opt)
    {
    case applied_operation_type::not_virt:
        return _impl->get_ops_history<filtered_not_virt_operations_history_index>(from_op, limit);
    case applied_operation_type::virt:
        return _impl->get_ops_history<filtered_virt_operations_history_index>(from_op, limit);
    case applied_operation_type::market:
        return _impl->get_ops_history<filtered_market_operations_history_index>(from_op, limit);
    default:;
    }
    return _impl->get_ops_history<operation_index>(from_op, limit);
}

std::map<uint32_t, applied_operation> blockchain_history_api::get_ops_in_block(uint32_t block_num,
                                                                               applied_operation_type opt) const
{
    using namespace deip::chain;

    const auto& db = _impl->_app.chain_database();

    return db->with_read_lock([&]() {
        const auto& idx = db->get_index<operation_index>().indices().get<by_location>();
        auto itr = idx.lower_bound(block_num);

        std::map<uint32_t, applied_operation> result;
        applied_operation temp;
        while (itr != idx.end() && itr->block == block_num)
        {
            auto id = itr->id;
            temp = *itr;
            if (detail::operation_filter(temp.op, opt))
            {
                FC_ASSERT(id._id >= 0, "Invalid operation_object id");
                result[(uint32_t)id._id] = temp;
            }
            ++itr;
        }
        return result;
    });
}
}
}
