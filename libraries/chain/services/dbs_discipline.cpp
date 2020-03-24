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

void dbs_discipline::increase_total_expertise_amount(const discipline_id_type& id, const share_type& amount)
{
    FC_ASSERT(amount >= 0, "Could not increase discipline total expertise amount with ${amount} value", ("amount", amount));
    const discipline_object& discipline = get_discipline(id);
    db_impl().modify(discipline, [&](discipline_object& d) {
        d.total_expertise_amount += amount; 
    });
}

const discipline_object& dbs_discipline::get_discipline_by_name(const fc::string& name) const
{
    const auto& idx = db_impl().get_index<discipline_index>().indices().get<by_discipline_name>();
    auto itr = idx.find(name, fc::strcmp_less());
    FC_ASSERT(itr != idx.end(), "Discipline: ${n} is not found", ("n", name));
    return *itr;
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

void dbs_discipline::increase_total_used_expertise_amount(const discipline_id_type& id, const share_type& amount)
{
    FC_ASSERT(amount >= 0, "Could not increase discipline total active weight with ${amount} value", ("amount", amount));
    const discipline_object& discipline = get_discipline(id);
    db_impl().modify(discipline, [&](discipline_object& d) {
        d.total_active_weight += amount;
    });
}

} //namespace chain
} //namespace deip