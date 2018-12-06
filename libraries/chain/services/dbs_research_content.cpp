#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/schema/research_object.hpp>


namespace deip{
namespace chain{


dbs_research_content::dbs_research_content(database &db) : _base_type(db)
{
}

const research_content_object& dbs_research_content::create(const research_id_type& research_id,
                                                            const research_content_type& type,
                                                            const std::string& title,
                                                            const std::string& content,
                                                            const std::string& permlink,
                                                            const std::vector<account_name_type>& authors,
                                                            const std::vector<research_content_id_type>& references,
                                                            const std::vector<string>& external_references)
{
    const auto& new_research_content = db_impl().create<research_content_object>([&](research_content_object& rc) {
        
        auto now = db_impl().head_block_time();
        
        rc.research_id = research_id;
        rc.type = type;
        fc::from_string(rc.title, title);
        fc::from_string(rc.content, content);
        fc::from_string(rc.permlink, permlink);
        rc.created_at = now;
        rc.authors.insert(authors.begin(), authors.end());
        rc.references.insert(references.begin(), references.end());
        rc.external_references.insert(external_references.begin(), external_references.end());
        rc.activity_round = 1;
        rc.activity_state = research_content_activity_state::active;

        if (type == research_content_type::announcement || 
                (type >= research_content_type::start_milestone_type && type <= research_content_type::last_milestone_type)) {

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

        auto& research = db_impl().get<research_object>(research_id);
        db_impl().modify(research, [&](research_object& r_o) { r_o.last_update_time = now; });

    });

    return new_research_content;
}

const research_content_object& dbs_research_content::get(const research_content_id_type& id) const
{
    try {
        return db_impl().get<research_content_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const research_content_object& dbs_research_content:: get_by_permlink(const research_id_type &research_id,
                                                                      const string &permlink) const
{
    const auto& idx = db_impl().get_index<research_content_index>().indices().get<by_permlink>();
    auto itr = idx.find(std::make_tuple(research_id, permlink));
    FC_ASSERT(itr != idx.end(), "Research content by permlink ${p} is not found", ("p", permlink));
    return *itr;
}

dbs_research_content::research_content_refs_type dbs_research_content::get_by_research_id(
        const research_id_type &research_id) const
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

dbs_research_content::research_content_refs_type dbs_research_content::get_by_research_and_type(
        const research_id_type &research_id, const research_content_type &type) const
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

dbs_research_content::research_content_refs_type dbs_research_content::get_all_milestones_by_research_id(const research_id_type& research_id) const
{
    research_content_refs_type ret;

    auto it_pair = db_impl().get_index<research_content_index>().indicies().get<by_research_id>().equal_range(research_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        if (it->is_milestone())
            ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

// Grant applications

const grant_application_object dbs_research_content::create_grant_application(const grant_id_type& grant_id,
                                                                              const research_id_type& research_id,
                                                                              const std::string& application_hash,
                                                                              const account_name_type& creator)
{
    const auto& new_grant_application = db_impl().create<grant_application_object>([&](grant_application_object& ga) {

        auto now = db_impl().head_block_time();

        ga.grant_id = grant_id;
        ga.research_id = research_id;
        fc::from_string(ga.application_hash, application_hash);
        ga.creator = creator;
        ga.created_at = now;
    });

    return new_grant_application;
}

const grant_application_object dbs_research_content::get_grant_application(const grant_application_id_type& id)
{
    try {
        return db_impl().get<grant_application_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

dbs_research_content::grant_applications_refs_type dbs_research_content::get_applications_by_grant(const grant_id_type& grant_id)
{
    grant_applications_refs_type ret;

    auto it_pair = db_impl().get_index<grant_application_index>().indicies().get<by_grant_id>().equal_range(grant_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_content::grant_applications_refs_type dbs_research_content::get_applications_by_research_id(const research_id_type& research_id)
{
    grant_applications_refs_type ret;

    auto it_pair = db_impl().get_index<grant_application_index>().indicies().get<by_research_id>().equal_range(research_id);
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