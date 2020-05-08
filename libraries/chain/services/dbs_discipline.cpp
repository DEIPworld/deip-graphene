#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_discipline::dbs_discipline(database &db)
    : _base_type(db)
{
}

dbs_discipline::discipline_ref_type dbs_discipline::get_disciplines() const
{
    discipline_ref_type ret;

    const auto& idx = db_impl().get_index<discipline_index>().indicies().get<by_id>();
    auto it = idx.lower_bound(0);
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const discipline_object& dbs_discipline::get_discipline(const discipline_id_type& id) const
{
    try {
        return db_impl().get<discipline_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_discipline::discipline_optional_ref_type dbs_discipline::get_discipline_if_exists(const discipline_id_type& id) const
{
    discipline_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<discipline_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const discipline_object& dbs_discipline::get_discipline_by_name(const fc::string& name) const
{
    const auto& idx = db_impl().get_index<discipline_index>().indices().get<by_discipline_name>();
    auto itr = idx.find(name, fc::strcmp_less());
    FC_ASSERT(itr != idx.end(), "Discipline: ${n} is not found", ("n", name));
    return *itr;
}

const dbs_discipline::discipline_optional_ref_type 
dbs_discipline::get_discipline_by_name_if_exists(const fc::string& name) const
{
    discipline_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<discipline_index>()
            .indicies()
            .get<by_discipline_name>();

    auto itr = idx.find(name, fc::strcmp_less());
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

void dbs_discipline::check_discipline_existence_by_name(const fc::string& name)
{
    const auto& idx = db_impl().get_index<discipline_index>().indices().get<by_discipline_name>();

    FC_ASSERT(idx.find(name, fc::strcmp_less()) != idx.cend(), "Discipline \"${1}\" does not exist", ("1", name));
}

void dbs_discipline::check_discipline_existence(const discipline_id_type &id)
{
    const auto& idx = db_impl().get_index<discipline_index>().indices().get<by_id>();

    FC_ASSERT(idx.find(id) != idx.cend(), "Discipline with id=\"${1}\" does not exist", ("1", id));
}

const bool dbs_discipline::discipline_exists(const discipline_id_type& id) const
{
    const auto& idx = db_impl()
            .get_index<discipline_index>()
            .indices()
            .get<by_id>();

    return idx.find(id) != idx.end();
}

dbs_discipline::discipline_ref_type dbs_discipline::get_disciplines_by_parent_id(const discipline_id_type parent_id) const
{
    discipline_ref_type ret;

    auto it_pair = db_impl().get_index<discipline_index>().indicies().get<by_parent_id>().equal_range(parent_id);
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