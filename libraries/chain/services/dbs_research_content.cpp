#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>

namespace deip{
namespace chain{


dbs_research_content::dbs_research_content(database &db) : _base_type(db)
{
}

const research_content_object& dbs_research_content::create_research_content(
                                                            const research_id_type& research_id,
                                                            const research_content_type& type,
                                                            const std::string& title,
                                                            const std::string& content,
                                                            const std::string& permlink,
                                                            const std::vector<account_name_type>& authors,
                                                            const std::vector<research_content_id_type>& references,
                                                            const std::set<string>& external_references)
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
        
        for (auto& str : external_references)
        {
            int val_length = str.length();
            char val_array[val_length + 1];
            strcpy(val_array, str.c_str());
            rc.external_references.insert(fc::shared_string(val_array, basic_string_allocator(db_impl().get_segment_manager())));
        }

        rc.activity_round = 1;
        rc.activity_state = research_content_activity_state::active;

        if (type == research_content_type::announcement ||  (type >= research_content_type::start_milestone_type && type <= research_content_type::last_milestone_type)) 
        {
            // the 1st activity period for intermediate result starts immediately 
            // after publishing and continues for 2 weeks
            rc.activity_window_start = now;
            rc.activity_window_end = now + DEIP_REGULAR_CONTENT_ACTIVITY_WINDOW_DURATION;

        }

        else if (type == research_content_type::final_result) {
            // the 1st activity period for final result starts immediately 
            // after publishing and continues for 2 months
            rc.activity_window_start = now;
            rc.activity_window_end = now + DEIP_FINAL_RESULT_ACTIVITY_WINDOW_DURATION;
        }

        const auto& research = db_impl().get<research_object>(research_id);
        db_impl().modify(research, [&](research_object& r_o)
        {
            r_o.last_update_time = now;
            r_o.contents_amount++;
        });

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

const research_content_object& dbs_research_content::get_by_permlink(const research_id_type &research_id,
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

const std::map<discipline_id_type, share_type> dbs_research_content::get_eci_evaluation(const research_content_id_type& research_content_id) const
{
    const dbs_review& review_service = db_impl().obtain_service<dbs_review>();
    const dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    const dbs_research_discipline_relation& research_discipline_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();

    const auto& research_content = get(research_content_id);
    const auto& research_content_reviews = review_service.get_reviews_by_research_content(research_content_id);
    const auto& research = research_service.get_research(research_content.research_id);
    const auto& research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);

    std::map<review_id_type, std::map<discipline_id_type, share_type>> reviews_weights;
    for (auto& wrap: research_content_reviews) 
    {
        const review_object& review = wrap.get();
        const auto review_weight_by_discipline = review_service.get_eci_weight(review.id);
        reviews_weights[review.id] = review_weight_by_discipline;
    }

    std::map<discipline_id_type, share_type> research_content_eci_by_discipline;
    for (auto& wrap: research_discipline_relations) 
    {
        const research_discipline_relation_object& relation = wrap.get();
        const discipline_id_type discipline_id = relation.discipline_id;

        const share_type Sdp = std::accumulate(reviews_weights.begin(), reviews_weights.end(), share_type(0), 
            [=](share_type acc, const std::pair<review_id_type, std::map<discipline_id_type, share_type>>& entry) {
                return entry.second.find(discipline_id) != entry.second.end() 
                    ? acc + entry.second.at(discipline_id)
                    : acc;
            });

        research_content_eci_by_discipline[discipline_id] = Sdp > 0 ? Sdp : 0;
    }

    return research_content_eci_by_discipline;
}

const research_content_object& dbs_research_content::update_eci_evaluation(const research_content_id_type& research_content_id)
{
    const auto& research_content = get(research_content_id);
    const auto& eci_evaluation = get_eci_evaluation(research_content_id);

    db_impl().modify(research_content, [&](research_content_object& rc_o) {
        for (auto& entry : eci_evaluation)
        {
            rc_o.eci_per_discipline[entry.first] = entry.second;
        }
    });

    return research_content;
}

}
}