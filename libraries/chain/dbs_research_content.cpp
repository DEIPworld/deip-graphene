#include <deip/chain/dbs_research_content.hpp>
#include <deip/chain/database.hpp>



namespace deip{
namespace chain{


dbs_research_content::dbs_research_content(database &db) : _base_type(db)
{
}

const research_content_object& dbs_research_content::create(const research_id_type& research_id,
                                                            const research_content_type& type,
                                                            const string& content,
                                                            const flat_set<account_name_type>& authors,
                                                            const std::vector<research_references_data>& research_references,
                                                            const std::vector<string>& research_external_references)
{
    int size = research_references.size();
    for (int i = 0; i < size; ++i)
    {
        FC_ASSERT(research_references[i].research_reference_id != research_id,
                  "Research material cannot reference research it is being created for.");
        if (research_references[i].research_content_reference_id.valid()) {
            check_research_content_existence(*research_references[i].research_content_reference_id);
            auto& research_content = db_impl().get<research_content_object>(*research_references[i].research_content_reference_id);
            FC_ASSERT(research_content.research_id == research_references[i].research_reference_id,
                      "Research content must be a part of specified research.");
        }
    }
    const auto& new_research_content = db_impl().create<research_content_object>([&](research_content_object& rc) {
        
        auto now = db_impl().head_block_time();
        
        rc.research_id = research_id;
        rc.type = type;
        rc.content = content;
        rc.authors = authors;
        rc.created_at = now;
        rc.research_references = research_references;
        rc.research_external_references = research_external_references;

        rc.activity_round = 1;
        rc.activity_state = research_content_activity_state::active;

        if (type == research_content_type::announcement || 
            type == research_content_type::milestone ||
            type == research_content_type::review) {

            // the 1st activity period for intermediate result starts immediately 
            // after publishing and continues for 2 weeks
            rc.activity_window_start = now;
            rc.activity_window_end = now + DAYS_TO_SECONDS(14);

        } else if (type == research_content_type::final_result) {
            // the 1st activity period for final result starts immediately 
            // after publishing and continues for 2 months
            rc.activity_window_start = now;
            rc.activity_window_end = now + DAYS_TO_SECONDS(60);
        }

    });

    return new_research_content;
}

const research_content_object& dbs_research_content::get_content_by_id(const research_content_id_type& id) const
{
    try 
    {
        return db_impl().get<research_content_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

dbs_research_content::research_content_refs_type dbs_research_content::get_content_by_research_id(const research_id_type& research_id) const
{
    research_content_refs_type ret;
    
    auto it_pair = db_impl().get_index<research_content_index>().indicies()
                            .get<by_research_id>()
                            .equal_range(research_id);
                            
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_content::research_content_refs_type dbs_research_content::get_content_by_research_id_and_content_type(const research_id_type& research_id, const research_content_type& type) const
{
    research_content_refs_type ret;

    auto it_pair = db_impl().get_index<research_content_index>().indicies()
                            .get<by_research_id_and_content_type>()
                            .equal_range(boost::make_tuple(research_id, type));

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_research_content::check_research_content_existence(const research_content_id_type& research_content_id)
{
    auto research_content = db_impl().find<research_content_object, by_id>(research_content_id);
    FC_ASSERT(research_content != nullptr, "Research content with id \"${1}\" must exist.", ("1", research_content_id));
}

}
}