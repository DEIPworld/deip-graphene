#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_license.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>

namespace deip {
namespace chain {

dbs_research_license::dbs_research_license(database &db) : _base_type(db)
{
}

const research_license_object& dbs_research_license::create_research_license(const research_object& research,
                                                                             const external_id_type& external_id,
                                                                             const account_name_type& licensee,
                                                                             const string& terms,
                                                                             const optional<time_point_sec>& expiration_time_opt,
                                                                             const optional<asset>& fee_opt)
{
    auto& dgp_service = db_impl().obtain_service<dbs_dynamic_global_properties>();
    const auto now = db_impl().head_block_time();

    const auto& research_license = db_impl().create<research_license_object>([&](research_license_object& rl_o) {
        rl_o.external_id = external_id;
        rl_o.research_external_id = research.external_id;
        rl_o.research_group = research.research_group;
        rl_o.licensee = licensee;

        fc::from_string(rl_o.terms, terms);

        rl_o.created_at = now;

        if (expiration_time_opt.valid())
        {
            rl_o.expiration_time = *expiration_time_opt;
        }
        else
        {
            rl_o.expiration_time = time_point_sec::maximum();
        }

        if (rl_o.created_at <= rl_o.expiration_time)
        {
            rl_o.status = static_cast<uint16_t>(research_license_status::active);
        }
        else
        {
            rl_o.status = static_cast<uint16_t>(research_license_status::expired);
        }

        if (fee_opt.valid())
        {
            rl_o.fee = *fee_opt;
        }
    });

    dgp_service.create_recent_entity(external_id);

    return research_license;
}

const dbs_research_license::research_license_refs_type dbs_research_license::get_research_licenses_by_research_group(const account_name_type& research_group) const
{
    research_license_refs_type ret;

    const auto& idx = db_impl()
      .get_index<research_license_index>()
      .indicies()
      .get<by_research_group>();

    auto it_pair = idx.equal_range(research_group);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_license_object& dbs_research_license::get_research_license(const external_id_type& external_id) const
{
    try { return db_impl().get<research_license_object, by_external_id>(external_id); }
    FC_CAPTURE_AND_RETHROW((external_id))
}

const dbs_research_license::research_license_optional_ref_type dbs_research_license::get_research_license_if_exists(const external_id_type& external_id) const
{
    research_license_optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<research_license_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}


const dbs_research_license::research_license_refs_type dbs_research_license::get_research_licenses_by_research(const external_id_type& research_external_id) const
{
    research_license_refs_type ret;

    const auto& idx = db_impl()
      .get_index<research_license_index>()
      .indicies()
      .get<by_research>();

    auto it_pair = idx.equal_range(research_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const dbs_research_license::research_license_refs_type dbs_research_license::get_research_licenses_by_licensee(const account_name_type& licensee) const
{
    research_license_refs_type ret;

    const auto& idx = db_impl()
      .get_index<research_license_index>()
      .indicies()
      .get<by_licensee>();

    auto it_pair = idx.equal_range(licensee);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const bool dbs_research_license::research_license_exists(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<research_license_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    return itr != idx.end();
}


const dbs_research_license::research_license_refs_type dbs_research_license::get_research_licenses_by_licensee_and_research(const account_name_type& licensee, const external_id_type& research_external_id) const
{
    research_license_refs_type ret;

    const auto& idx = db_impl()
      .get_index<research_license_index>()
      .indicies()
      .get<by_licensee_and_research>();

    auto it_pair = idx.equal_range(boost::make_tuple(licensee, research_external_id));

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const dbs_research_license::research_license_refs_type dbs_research_license::get_research_licenses_by_licensee_and_research_group(const account_name_type& licensee, const account_name_type& research_group) const
{
    research_license_refs_type ret;

    const auto& idx = db_impl()
      .get_index<research_license_index>()
      .indicies()
      .get<by_licensee_and_research_group>();

    auto it_pair = idx.equal_range(boost::make_tuple(licensee, research_group));

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


}
}