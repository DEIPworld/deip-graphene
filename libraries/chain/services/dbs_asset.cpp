#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_research.hpp>
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
                                            const string& description,
                                            const optional<std::reference_wrapper<const research_object>>& tokenized_research,
                                            const optional<percent>& license_revenue_holders_share)
{

    FC_ASSERT(current_supply <= max_supply, "Current supply can not be greater than Max supply");

    const auto& asset_o = db_impl().create<asset_object>([&](asset_object& asset) {
        asset.symbol = symbol;
        asset.precision = precision;
        asset.issuer = issuer;
        asset.current_supply = current_supply;
        asset.max_supply = max_supply;
        fc::from_string(asset.string_symbol, string_symbol);
        fc::from_string(asset.description, description);
        asset.type = static_cast<uint8_t>(asset_type::basic);

        if (tokenized_research.valid())
        {
            const research_object& research = *tokenized_research;
            asset.tokenized_research = research.external_id;
            asset.type = static_cast<uint8_t>(asset_type::research_security_token);
        }

        if (license_revenue_holders_share.valid())
        {
            asset.license_revenue_holders_share = *license_revenue_holders_share;
        }
    });

    return asset_o;
}

const asset_object& dbs_asset::issue_asset(const asset_object& asset_o, const account_name_type& recipient, const asset& amount)
{
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    auto& research_service = db_impl().obtain_service<dbs_research>();

    account_balance_service.adjust_account_balance(recipient, amount);
    adjust_asset_current_supply(asset_o, amount);

    if (asset_type::research_security_token == static_cast<asset_type>(asset_o.type))
    {
        const auto& tokenized_research = research_service.get_research(*asset_o.tokenized_research);
        db_impl().modify(tokenized_research, [&](research_object& r_o) {
            const auto& itr = std::find_if(r_o.security_tokens.begin(), r_o.security_tokens.end(),
                [&](const asset& a) { return a.symbol == asset_o.symbol; });

            if (itr != r_o.security_tokens.end())
            {
                itr->amount += amount.amount;
            } 
            else 
            {
                r_o.security_tokens.insert(amount);
            }
        });
    }

    return asset_o;
}


const asset_object& dbs_asset::reserve_asset(const asset_object& asset_o, const account_name_type& owner, const asset& amount)
{
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    auto& research_service = db_impl().obtain_service<dbs_research>();

    account_balance_service.adjust_account_balance(owner, -amount);
    adjust_asset_current_supply(asset_o, -amount);

    if (asset_type::research_security_token == static_cast<asset_type>(asset_o.type))
    {
        const auto& tokenized_research = research_service.get_research(*asset_o.tokenized_research);
        db_impl().modify(tokenized_research, [&](research_object& r_o) {
            const auto& itr = std::find_if(r_o.security_tokens.begin(), r_o.security_tokens.end(),
              [&](const asset& a) { return a.symbol == asset_o.symbol; });

            if (itr != r_o.security_tokens.end())
            {
                itr->amount -= amount.amount;
            }
        });
    }

    return asset_o;
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
    const auto& idx = db_impl()
      .get_index<asset_index>()
      .indices()
      .get<by_symbol>();
   
    const auto& itr = idx.find(symbol);
    FC_ASSERT(itr != idx.end(), "Asset ${1} is not found", ("1", symbol));
    return *itr;
}

const asset_object& dbs_asset::adjust_asset_current_supply(const asset_object& asset_o, const asset& delta)
{
    FC_ASSERT(asset_o.current_supply + delta.amount >= 0, "You cannot adjust supply on this value.");
    FC_ASSERT(asset_o.current_supply + delta.amount <= asset_o.max_supply, "Max supply limit is reached");

    db_impl().modify(asset_o, [&](asset_object& a_o) { a_o.current_supply += delta.amount; });

    return asset_o;
}

const asset_object& dbs_asset::get_asset_by_string_symbol(const string& string_symbol) const
{
    const auto& idx = db_impl()
      .get_index<asset_index>()
      .indices()
      .get<by_string_symbol>();

    const auto& itr = idx.find(string_symbol, fc::strcmp_less());
    FC_ASSERT(itr != idx.end(), "Asset ${1} is not found", ("1", string_symbol));
    return *itr;
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

const dbs_asset::asset_refs_type dbs_asset::get_assets_by_issuer(const account_name_type& issuer) const
{
    asset_refs_type ret;

    auto it_pair = db_impl()
      .get_index<asset_index>()
      .indicies()
      .get<by_issuer>()
      .equal_range(issuer);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const dbs_asset::asset_refs_type dbs_asset::lookup_assets(const string& lower_bound_symbol, uint32_t limit) const
{
    asset_refs_type ret;

    const auto& assets_by_symbol = db_impl()
      .get_index<asset_index>()
      .indices()
      .get<by_string_symbol>();

    for (auto itr = assets_by_symbol.lower_bound(lower_bound_symbol, fc::strcmp_less()); limit-- && itr != assets_by_symbol.end(); ++itr)
    {
        ret.push_back(std::cref(*itr));
    }

    return ret;
}

const dbs_asset::asset_refs_type dbs_asset::get_assets_by_type(const asset_type& type) const
{
    asset_refs_type ret;

    const uint8_t num_val = static_cast<uint8_t>(type);

    auto it_pair = db_impl()
      .get_index<asset_index>()
      .indicies()
      .get<by_type>()
      .equal_range(num_val);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const dbs_asset::asset_refs_type dbs_asset::get_assets_by_tokenize_research(const optional<external_id_type>& tokenized_research) const
{
    asset_refs_type ret;

    auto it_pair = db_impl()
      .get_index<asset_index>()
      .indicies()
      .get<by_tokenized_research>()
      .equal_range(tokenized_research);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


} //namespace chain
} //namespace deip