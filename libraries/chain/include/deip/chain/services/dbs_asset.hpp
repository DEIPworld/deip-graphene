#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/asset_object.hpp>

namespace deip {
namespace chain {

using protocol::asset_symbol_type;

class dbs_asset : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_asset() = delete;

protected:
    explicit dbs_asset(database &db);

public:
    using asset_optional_ref_type = fc::optional<std::reference_wrapper<const asset_object>>;

    const asset_object& create_asset(const account_name_type& issuer,
                                     const asset_symbol_type& symbol,
                                     const string& string_symbol,
                                     const uint8_t& precision,
                                     const share_type& current_supply,
                                     const share_type& max_supply,
                                     const string& description);

    const asset_object& get_asset(const asset_id_type& id) const;

    const asset_optional_ref_type get_asset_if_exists(const asset_id_type& id) const;

    const asset_object& get_asset_by_symbol(const asset_symbol_type& symbol) const;

    const asset_optional_ref_type get_asset_by_symbol_if_exists(const asset_symbol_type& symbol) const;

    const asset_object& get_asset_by_string_symbol(const string& string_symbol) const;

    const asset_optional_ref_type get_asset_by_string_symbol_if_exists(const string& string_symbol) const;

    const bool asset_exists_by_symbol(const asset_symbol_type& symbol) const;

    const asset_object& adjust_asset_current_supply(const asset_object& asset_o, const share_type& delta);

};

} // namespace chain
} // namespace deip