#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/asset_object.hpp>

namespace deip {
namespace chain {

class dbs_asset : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_asset() = delete;

protected:
    explicit dbs_asset(database &db);

public:
    const asset_object& create(const protocol::asset_symbol_type& symbol,
                               const std::string& string_symbol,
                               const uint8_t& precision,
                               const account_name_type& issuer,
                               const string& name,
                               const string& description);

    const asset_object& get(const asset_id_type& id) const;

    bool exists_by_symbol(const protocol::asset_symbol_type& symbol) const;

    void check_existence(const protocol::asset_symbol_type& symbol) const;

    const asset_object& get_by_symbol(const protocol::asset_symbol_type& symbol) const;

    void adjust_current_supply(const asset_object& asset_obj, const share_type& delta);

    const asset_object& get_by_string_symbol(const std::string& string_symbol) const;
};

} // namespace chain
} // namespace deip