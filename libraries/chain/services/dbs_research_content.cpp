#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>

namespace deip{
namespace chain{


dbs_research_content::dbs_research_content(database &db) : _base_type(db)
{
}

const research_content_object& dbs_research_content::create_research_content(
  const research_object& research,
  const external_id_type& external_id,
  const std::string& description,
  const std::string& content,
  const research_content_type& type,
  const flat_set<account_name_type>& authors,
  const flat_set<external_id_type>& references,
  const fc::time_point_sec& timestamp) 
{
    auto& dgp_service = db_impl().obtain_service<dbs_dynamic_global_properties>();

    const auto& research_content = db_impl().create<research_content_object>([&](research_content_object& rc_o) {
        rc_o.research_id = research.id;
        rc_o.research_external_id = research.external_id;
        rc_o.external_id = external_id;
        fc::from_string(rc_o.description, description);
        fc::from_string(rc_o.content, content);
        rc_o.type = type;
        rc_o.authors.insert(authors.begin(), authors.end());
        rc_o.references.insert(references.begin(), references.end());   
        rc_o.created_at = timestamp;

        // deprecated
        rc_o.activity_round = 1;
        rc_o.activity_state = research_content_activity_state::active;
        rc_o.activity_window_start = timestamp;
        rc_o.activity_window_end = time_point_sec::maximum();
    });

    db_impl().modify(research, [&](research_object& r_o) { 
      r_o.last_update_time = timestamp;
      r_o.number_of_research_contents += 1;
    });

    dgp_service.create_recent_entity(external_id);

    return research_content;
}

const research_content_object& dbs_research_content::get_research_content(const research_content_id_type& id) const
{
    try { return db_impl().get<research_content_object, by_id>(id); }
    FC_CAPTURE_AND_RETHROW((id))
}

const research_content_object& dbs_research_content::get_research_content(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<research_content_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    FC_ASSERT(itr != idx.end(), "Research content with external id ${1} does not exist", ("1", external_id));
    return *itr;
}

const dbs_research_content::research_content_optional_ref_type dbs_research_content::get_research_content_if_exists(const external_id_type& external_id) const
{
    research_content_optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<research_content_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const dbs_research_content::research_content_optional_ref_type dbs_research_content::get_research_content_if_exists(const research_content_id_type& id) const
{
    research_content_optional_ref_type result;

    const auto& idx = db_impl()
            .get_index<research_content_index>()
            .indices()
            .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}



dbs_research_content::research_content_refs_type dbs_research_content::get_research_content_by_research_id(const research_id_type& research_id) const
{
    research_content_refs_type ret;
    
    const auto& idx = db_impl()
      .get_index<research_content_index>()
      .indicies()
      .get<by_research_id>();

    auto it_pair = idx.equal_range(research_id);
    
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
  const research_id_type& research_id, 
  const research_content_type& type) const
{
    research_content_refs_type ret;

    const auto& idx = db_impl()
      .get_index<research_content_index>()
      .indicies()
      .get<by_research_id_and_content_type>();

    auto it_pair = idx.equal_range(boost::make_tuple(research_id, type));
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

// Grant applications

const std::map<discipline_id_type, share_type> dbs_research_content::get_eci_evaluation(const research_content_id_type& research_content_id) const
{
    const dbs_review& review_service = db_impl().obtain_service<dbs_review>();
    const dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    const dbs_research_discipline_relation& research_discipline_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();

    const auto& research_content = get_research_content(research_content_id);
    const auto& research_content_reviews = review_service.get_reviews_by_research_content(research_content_id);
    const auto& research = research_service.get_research(research_content.research_id);
    const auto& research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);

    std::map<review_id_type, std::map<discipline_id_type, share_type>> reviews_weights;
    for (const review_object& review : research_content_reviews)
    {
        const auto review_weight_by_discipline = review_service.get_eci_weight(review.id);
        reviews_weights[review.id] = review_weight_by_discipline;
    }

    std::map<discipline_id_type, share_type> research_content_eci_by_discipline;
    for (const research_discipline_relation_object& relation : research_discipline_relations)
    {
        const discipline_id_type discipline_id = relation.discipline_id;

        const share_type Sdp = std::accumulate(reviews_weights.begin(), reviews_weights.end(), share_type(0), 
            [=](share_type acc, const std::pair<review_id_type, std::map<discipline_id_type, share_type>>& entry) {
                return entry.second.find(discipline_id) != entry.second.end() 
                    ? acc + entry.second.at(discipline_id)
                    : acc;
            });

        research_content_eci_by_discipline[discipline_id] = Sdp;
    }

    return research_content_eci_by_discipline;
}

const research_content_object& dbs_research_content::update_eci_evaluation(const research_content_id_type& research_content_id)
{
    const auto& research_content = get_research_content(research_content_id);
    const auto& eci_evaluation = get_eci_evaluation(research_content_id);

    db_impl().modify(research_content, [&](research_content_object& rc_o) {
        for (const auto& entry : eci_evaluation)
        {
            rc_o.eci_per_discipline[entry.first] = entry.second;
        }
    });

    return research_content;
}

const dbs_research_content::research_content_refs_type dbs_research_content::lookup_research_contents(const research_content_id_type& lower_bound, uint32_t limit) const
{
    research_content_refs_type result;

    const auto& idx = db_impl()
      .get_index<research_content_index>()
      .indicies()
      .get<by_id>();

    for (auto itr = idx.lower_bound(lower_bound); limit-- && itr != idx.end(); ++itr)
    {
        result.push_back(std::cref(*itr));
    }

    return result;
}

}
}