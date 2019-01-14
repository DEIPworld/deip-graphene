#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_asset::dbs_asset(database &db)
    : _base_type(db)
{
}

const asset_object& dbs_asset::create(const asset_symbol_type& symbol,
                                      const std::string& string_symbol,
                                      const uint8_t& precision,
                                      const account_name_type& issuer,
                                      const string& name,
                                      const string& description)
{

    auto& asset = db_impl().create<asset_object>([&](asset_object& asset) {
        asset.symbol = symbol;
        fc::from_string(asset.string_symbol, string_symbol);
        asset.precision = precision;
        asset.issuer = issuer;
        fc::from_string(asset.name, name);
        fc::from_string(asset.description, description);
    });

    return asset;
}

const asset_object& dbs_asset::get(const asset_id_type& id) const
{
    return db_impl().get<asset_object>(id);
}

bool dbs_asset::exists_by_symbol(const protocol::asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<asset_index>().indices().get<by_symbol>();
    auto itr = idx.find(symbol);
    if (itr != idx.end())
        return true;
    else
        return false;
}

void dbs_asset::check_existence(const protocol::asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<asset_index>().indices().get<by_symbol>();
    FC_ASSERT(idx.find(symbol) != idx.cend(), "Asset \"${1}\" does not exist.", ("1", symbol));
}

const asset_object& dbs_asset::get_by_symbol(const protocol::asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<asset_index>().indices().get<by_symbol>();
    return *idx.find(symbol);
}

void dbs_asset::adjust_current_supply(const asset_object& asset_obj, const share_type& delta)
{
    FC_ASSERT(asset_obj.current_supply + delta >= 0, "You cannot adjust supply on this value.");

    db_impl().modify(asset_obj, [&](asset_object& a_o) { a_o.current_supply += delta; });
}

const asset_object& dbs_asset::get_by_string_symbol(const std::string& string_symbol) const
{
    const auto& idx = db_impl().get_index<asset_index>().indices().get<by_string_symbol>();
    return *idx.find(string_symbol, fc::strcmp_less());
}

} //namespace chain
} //namespace deip