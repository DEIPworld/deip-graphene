#include <deip/chain/dbs_research.hpp>
#include <deip/chain/database.hpp>


namespace deip{
namespace chain{

dbs_research::dbs_research(database &db) : _base_type(db)
{
}


dbs_research::research_refs_type dbs_research::get_researches() const
{
    research_refs_type ret;

    auto idx = db_impl().get_index<research_index>().indicies();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;

}

const research_object& dbs_research::get_research(const research_id_type id) const
{
    return db_impl().get<research_object>(id);
}

const research_object& dbs_research::get_research_by_permlink(const string & permlink) const
{
    return db_impl().get<research_object, by_permlink>(permlink);
}

const research_object& dbs_research::get_research_by_discipline_id(const discipline_id_type& discipline_ids) const
{
    return db_impl().get<research_object, by_discipline_id>(discipline_ids);
}

}
}