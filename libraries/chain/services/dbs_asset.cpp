#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

using protocol::asset_symbol_type;

dbs_asset::dbs_asset(database &db)
    : _base_type(db)
{
}

const asset_object& dbs_asset::create_asset(const account_name_type& issuer,
                                            const asset_symbol_type& symbol,
                                            const std::string& string_symbol,
                                            const uint8_t& precision,
                                            const share_type& current_supply,
                                            const share_type& max_supply,
                                            const string& description)
{

    const auto& asset = db_impl().create<asset_object>([&](asset_object& asset) {
        asset.symbol = symbol;
        asset.precision = precision;
        asset.issuer = issuer;
        asset.current_supply = current_supply;
        asset.max_supply = max_supply;
        fc::from_string(asset.string_symbol, string_symbol);
        fc::from_string(asset.description, description);
    });

    return asset;
}

const asset_object& dbs_asset::get_asset(const asset_id_type& id) const
{
    return db_impl().get<asset_object>(id);
}

const dbs_asset::asset_optional_ref_type
dbs_asset::get_asset_if_exists(const asset_id_type& id) const
{
    asset_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<asset_index>()
            .indicies()
            .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const bool dbs_asset::asset_exists_by_symbol(const asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<asset_index>().indices().get<by_symbol>();
    auto itr = idx.find(symbol);
    return itr != idx.end();
}

const asset_object& dbs_asset::get_asset_by_symbol(const asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<asset_index>().indices().get<by_symbol>();
    return *idx.find(symbol);
}

const asset_object& dbs_asset::adjust_asset_current_supply(const asset_object& asset_o, const share_type& delta)
{
    FC_ASSERT(asset_o.current_supply + delta >= 0, "You cannot adjust supply on this value.");
    FC_ASSERT(asset_o.current_supply + delta <= asset_o.max_supply, "Max supply limit is reached");

    db_impl().modify(asset_o, [&](asset_object& a_o) { a_o.current_supply += delta; });

    return asset_o;
}

const asset_object& dbs_asset::get_asset_by_string_symbol(const string& string_symbol) const
{
    const auto& idx = db_impl().get_index<asset_index>().indices().get<by_string_symbol>();
    return *idx.find(string_symbol, fc::strcmp_less());
}

const dbs_asset::asset_optional_ref_type dbs_asset::get_asset_by_symbol_if_exists(const asset_symbol_type& symbol) const
{
    asset_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<asset_index>()
      .indicies()
      .get<by_symbol>();

    auto itr = idx.find(symbol);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}


const dbs_asset::asset_optional_ref_type dbs_asset::get_asset_by_string_symbol_if_exists(const std::string& string_symbol) const
{
    asset_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<asset_index>()
            .indicies()
            .get<by_string_symbol>();

    auto itr = idx.find(string_symbol, fc::strcmp_less());
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

} //namespace chain
} //namespace deip