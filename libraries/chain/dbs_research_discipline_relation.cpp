#include <deip/chain/dbs_research_discipline_relation.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_research_discipline_relation::dbs_research_discipline_relation(database &db)
        : _base_type(db)
{
}

const research_discipline_relation_object& dbs_research_discipline_relation::create(const research_id_type& research_id, const discipline_id_type& discipline_id)
{
    FC_ASSERT(discipline_id != 0, "Research cannot be in root discipline");

    const auto& new_relation = db_impl().create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
        r.research_id = research_id;
        r.discipline_id = discipline_id;
    });

    return new_relation;
}

const research_discipline_relation_object& dbs_research_discipline_relation::update_votes_count(const research_id_type& research_id, const discipline_id_type& discipline_id, int16_t delta)
{
    auto& relation = get_research_discipline_relations_by_research_and_discipline(research_id, discipline_id);

    FC_ASSERT(relation.votes_count + delta >= 0, "Votes amount cannot be negative");

    db_impl().modify(relation, [&](research_discipline_relation_object& r) {
        r.votes_count += delta;
    });

    return relation;
}

const research_discipline_relation_object& dbs_research_discipline_relation::get_research_discipline_relation(const research_discipline_relation_id_type& id) const
{
    try {
        return db_impl().get<research_discipline_relation_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}


dbs_research_discipline_relation::research_discipline_relation_refs_type dbs_research_discipline_relation::get_research_discipline_relations_by_research(const research_id_type& research_id) const
{
    research_discipline_relation_refs_type ret;

    auto it_pair = db_impl().get_index<research_discipline_relation_index>().indices().get<by_research_id>().equal_range(research_id);
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

    auto it_pair = db_impl().get_index<research_discipline_relation_index>().indices().get<by_discipline_id>().equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const research_discipline_relation_object& dbs_research_discipline_relation::get_research_discipline_relations_by_research_and_discipline(const research_id_type& research_id, const discipline_id_type& discipline_id) const
{
    try {
        return db_impl().get<research_discipline_relation_object, by_research_and_discipline>(boost::make_tuple(research_id, discipline_id));
    }
    FC_CAPTURE_AND_RETHROW((research_id)(discipline_id))
}

} //namespace chain
} //namespace deip