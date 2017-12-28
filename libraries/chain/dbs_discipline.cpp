#include <deip/chain/dbs_discipline.hpp>
#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_discipline::dbs_discipline(database &db)
    : _base_type(db)
{
}

dbs_discipline::discipline_refs_type dbs_discipline::get_disciplines() const
{
    discipline_refs_type ret;

    auto idx = db_impl().get_index<discipline_index>().indicies();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const discipline_object& dbs_discipline::get_discipline(discipline_id_type id) const
{
    return db_impl().get<discipline_object>(id);
}

const discipline_object& dbs_discipline::get_discipline_by_name(discipline_name_type name) const
{
    return db_impl().get<discipline_object, by_discipline_name>(name);
}

} //namespace chain
} //namespace deip