#include <deip/chain/dbs_discipline.hpp>
#include <deip/chain/database.hpp>

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
    return db_impl().get<discipline_object>(id);
}

const discipline_object& dbs_discipline::get_discipline_by_name(const discipline_name_type& name) const
{
    return db_impl().get<discipline_object, by_discipline_name>(name);
}

void dbs_discipline::check_discipline_existence_by_name(const discipline_name_type &name)
{
    const auto& idx = db_impl().get_index<discipline_index>().indices().get<by_discipline_name>();

    FC_ASSERT(idx.find(name) != idx.cend(), "Discipline \"${1}\" does not exist", ("1", name));
}

void dbs_discipline::check_discipline_existence(const discipline_id_type &id)
{
    const auto& idx = db_impl().get_index<discipline_index>().indices().get<by_id>();

    FC_ASSERT(idx.find(id) != idx.cend(), "Discipline with id=\"${1}\" does not exist", ("1", id));
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

const discipline_object& dbs_discipline::increase_total_active_research_reward_weight(const discipline_id_type& discipline_id,
                                                                                      const share_type amount)
{
    auto& discipline = db_impl().get<discipline_object, by_id>(discipline_id);
    db_impl().modify(discipline, [&](discipline_object& d_o) { d_o.total_active_research_reward_weight += amount; });
}

} //namespace chain
} //namespace deip