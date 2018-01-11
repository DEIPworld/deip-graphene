#include <deip/chain/dbs_research_discipline_relation.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_research_discipline_relation::dbs_research_discipline_relation(database &db)
        : _base_type(db)
{
}

const research_discipline_relation_object& dbs_research_discipline_relation::get_research_discipline_relation(const research_discipline_relation_id_type& id) const
{
    return db_impl().get<research_discipline_relation_object>(id);
}


dbs_research_discipline_relation::research_discipline_relation_refs_type dbs_research_discipline_relation::get_research_discipline_relations_by_research(const research_id_type& research_id) const
{
    research_discipline_relation_refs_type ret;

    auto it_pair = db_impl().get_index<research_discipline_relation_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_discipline_relation::research_discipline_relation_refs_type dbs_research_discipline_relation::get_research_discipline_relations_by_discipline(const discipline_id_type& discipline_id) const
{
    research_discipline_relation_refs_type ret;

    auto it_pair = db_impl().get_index<research_discipline_relation_index>().indicies().get<by_discipline_id>().equal_range(discipline_id);
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