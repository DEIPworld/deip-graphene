#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

/* [DEPRECATED] */
dbs_research_group::dbs_research_group(database& db)
  : _base_type(db)
{
}

const research_group_object& dbs_research_group::get_research_group(const research_group_id_type& id) const 
{
  try { return db_impl().get<research_group_object, by_id>(id); }
  FC_CAPTURE_AND_RETHROW((id))
}

const research_group_object& dbs_research_group::get_research_group(const account_name_type& id) const 
{
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_account>();

    auto itr = idx.find(id);
    FC_ASSERT(itr != idx.end(), "Research group account \"${1}\" does not exist.", ("1", id));
    return *itr;
}

const dbs_research_group::research_group_optional_ref_type dbs_research_group::get_research_group_if_exists(const research_group_id_type& id) const
{
    research_group_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}



const research_group_object& dbs_research_group::get_research_group_by_account(const account_name_type& account) const
{
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_account>();

    auto itr = idx.find(account);
    FC_ASSERT(itr != idx.end(), "Research group account \"${1}\" does not exist.", ("1", account));
    return *itr;
}

const dbs_research_group::research_group_optional_ref_type dbs_research_group::get_research_group_by_account_if_exists(const account_name_type& account) const
{
    research_group_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_account>();

    auto itr = idx.find(account);
    if (itr != idx.end())
    {
        result = *itr;
    }
    return result;
}

const research_group_object& dbs_research_group::create_personal_research_group(const account_name_type& account)
{
    const research_group_object& personal_research_group
        = db_impl().create<research_group_object>([&](research_group_object& rg_o) {
              rg_o.account = account;
              rg_o.creator = account;
              fc::from_string(rg_o.description, account);
              rg_o.management_model_v = 2;
              rg_o.is_personal = true;
              rg_o.is_centralized = false;
              rg_o.is_dao = false;
              rg_o.is_created_by_organization = false;
              rg_o.has_organization = false;
          });

    return personal_research_group;
}

const research_group_object& dbs_research_group::create_research_group(
  const account_name_type& account,
  const account_name_type& creator,
  const string& description)
{
    dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    dbs_account& account_service = db_impl().obtain_service<dbs_account>();

    const auto& block_time = db_impl().head_block_time();

    const research_group_object& research_group
        = db_impl().create<research_group_object>([&](research_group_object& rg_o) {
              rg_o.account = account;
              rg_o.creator = creator;
              fc::from_string(rg_o.description, description);
              rg_o.is_personal = false;
              rg_o.is_centralized = false;
              rg_o.is_dao = true;
              rg_o.default_quorum = DEIP_100_PERCENT;
          });

    const string default_research_external_id(account);
    const string default_research_description(account);
    const std::set<discipline_id_type>& default_research_disciplines = std::set<discipline_id_type>({});
    const optional<percent> default_research_review_share;
    const optional<percent> default_research_compensation_share;
    const bool& default_research_is_private = false;
    const bool& default_research_is_finished = false;
    const bool& default_research_is_default = true;

    const auto& research_account = account_service.get_account(account);

    const auto& default_research = research_service.create_research(
      research_account,
      external_id_type((string)fc::ripemd160::hash(default_research_external_id)),
      default_research_description,
      default_research_disciplines,
      default_research_is_private,
      default_research_is_finished,
      default_research_is_default,
      block_time
    );

    return research_group;
}

const research_group_object& dbs_research_group::update_research_group(
  const research_group_object& research_group,
  const string& description) 
{
    db_impl().modify(research_group, [&](research_group_object& rg_o) {
        fc::from_string(rg_o.description, description);
    });

    return research_group;
}

const bool dbs_research_group::research_group_exists(const research_group_id_type& research_group_id) const
{
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_id>();

    return idx.find(research_group_id) != idx.end();
}

const bool dbs_research_group::research_group_exists(const account_name_type& account) const
{
    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indices()
      .get<by_account>();

    return idx.find(account) != idx.end();
}

const dbs_research_group::research_group_refs_type dbs_research_group::lookup_research_groups(const research_group_id_type& lower_bound, uint32_t limit) const
{
    research_group_refs_type result;

    const auto& idx = db_impl()
      .get_index<research_group_index>()
      .indicies()
      .get<by_id>();

    for (auto itr = idx.lower_bound(lower_bound); limit-- && itr != idx.end(); ++itr)
    {
        result.push_back(std::cref(*itr));
    }

    return result;
}


} // namespace chain
} // namespace deip
