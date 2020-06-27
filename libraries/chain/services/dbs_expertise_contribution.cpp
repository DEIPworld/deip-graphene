#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/database/database.hpp>
#include <boost/lambda/lambda.hpp>
#include <tuple>

namespace deip {
namespace chain {

dbs_expertise_contribution::dbs_expertise_contribution(database& db)
    : _base_type(db)
{
}

const expertise_contribution_object& dbs_expertise_contribution::adjust_expertise_contribution(
  const discipline_id_type& discipline_id,
  const research_id_type& research_id,
  const research_content_id_type& research_content_id,
  const eci_diff& diff)
{

    if (expertise_contribution_exists(research_content_id, discipline_id))
    {
        const expertise_contribution_object& expertise_contribution = get_expertise_contribution_by_research_content_and_discipline(research_content_id, discipline_id);
        db_impl().modify(expertise_contribution, [&](expertise_contribution_object& ec_o) {
            ec_o.eci_current_block_delta += diff.diff();
            ec_o.eci = diff.current();
            ec_o.eci_current_block_diffs.push_back(diff);
            ec_o.has_eci_current_block_diffs = true;

            for (const auto& criteria : diff.assessment_criterias)
            {
                uint16_t score = diff.is_increased() ? criteria.second : -criteria.second;
                ec_o.assessment_criterias.insert(std::make_pair(criteria.first, score));
            }
        });

        return expertise_contribution;
    }

    else 
    {
        const expertise_contribution_object& expertise_contribution
            = db_impl().create<expertise_contribution_object>([&](expertise_contribution_object& ec_o) {
                  ec_o.discipline_id = discipline_id;
                  ec_o.research_id = research_id;
                  ec_o.research_content_id = research_content_id;
                  ec_o.eci_current_block_delta = diff.current();
                  ec_o.eci = diff.current();
                  ec_o.eci_current_block_diffs.push_back(diff);
                  ec_o.has_eci_current_block_diffs = true;

                  for (const auto& criteria : diff.assessment_criterias)
                  {
                      if (ec_o.assessment_criterias.find(criteria.first) != ec_o.assessment_criterias.end())
                      {
                          uint16_t score = diff.is_increased() ? criteria.second : -criteria.second;
                          ec_o.assessment_criterias.at(criteria.first) += score;
                      } 
                      else 
                      {
                          uint16_t score = diff.is_increased() ? criteria.second : -criteria.second;
                          ec_o.assessment_criterias.insert(std::make_pair(criteria.first, score));
                      }
                  }
              });

        return expertise_contribution;
    } 
}

const expertise_contribution_object& dbs_expertise_contribution::get_expertise_contribution(
  const expertise_contribution_id_type& id) const
{
    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);

    FC_ASSERT(itr != idx.end(), "Expertise contribution with id ${1} does not exist", ("1", id));

    return *itr;
}

const dbs_expertise_contribution::expertise_contribution_optional_ref_type
dbs_expertise_contribution::get_expertise_contribution_if_exists(const expertise_contribution_id_type& id) const
{
    expertise_contribution_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<expertise_contribution_index>()
            .indicies()
            .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const expertise_contribution_object& dbs_expertise_contribution::get_expertise_contribution_by_research_content_and_discipline(
  const research_content_id_type& research_content_id,
  const discipline_id_type& discipline_id) const
{
    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_research_content_and_discipline>();

    auto itr = idx.find(std::make_tuple(research_content_id, discipline_id));

    FC_ASSERT(itr != idx.end(), 
      "Expertise contribution to ${1} discipline for ${2} research content does not exist",
      ("1", discipline_id)("2", research_content_id));

    return *itr;
}

const dbs_expertise_contribution::expertise_contribution_optional_ref_type
dbs_expertise_contribution::get_expertise_contribution_by_research_content_and_discipline_if_exists(const research_content_id_type& research_content_id,
                                                                                                    const discipline_id_type& discipline_id) const
{
    expertise_contribution_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<expertise_contribution_index>()
            .indicies()
            .get<by_research_content_and_discipline>();

    auto itr = idx.find(std::make_tuple(research_content_id, discipline_id));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_expertise_contribution::expertise_contributions_refs_type dbs_expertise_contribution::get_expertise_contributions_by_research_and_discipline(
  const research_id_type& research_id,
  const discipline_id_type& discipline_id) const
{
    expertise_contributions_refs_type ret;

    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_research_and_discipline>();

    auto it_pair = idx.equal_range(std::make_tuple(research_id, discipline_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_expertise_contribution::expertise_contributions_refs_type dbs_expertise_contribution::get_expertise_contributions_by_research(
  const research_id_type& research_id) const
{
    expertise_contributions_refs_type ret;
    
    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
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

dbs_expertise_contribution::expertise_contributions_refs_type dbs_expertise_contribution::get_expertise_contributions_by_discipline(
  const discipline_id_type& discipline_id) const
{
    expertise_contributions_refs_type ret;

    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_discipline_id>();

    auto it_pair = idx.equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_expertise_contribution::expertise_contributions_refs_type dbs_expertise_contribution::get_expertise_contributions_by_research_content(
  const research_content_id_type& research_content_id) const
{
    expertise_contributions_refs_type ret;

    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_research_content_id>();

    auto it_pair = idx.equal_range(research_content_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}


const bool dbs_expertise_contribution::expertise_contribution_exists(
  const research_content_id_type& research_content_id,
  const discipline_id_type& discipline_id) const
{
    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_research_content_and_discipline>();

    auto itr = idx.find(std::make_tuple(research_content_id, discipline_id));
    return itr != idx.end();
}

dbs_expertise_contribution::expertise_contributions_refs_type dbs_expertise_contribution::get_altered_expertise_contributions_in_block() const
{
    expertise_contributions_refs_type ret;

    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_has_eci_current_block_diffs>();

    auto it_pair = idx.equal_range(true);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_expertise_contribution::expertise_contributions_refs_type dbs_expertise_contribution::get_increased_expertise_contributions_in_block() const
{
    expertise_contributions_refs_type ret;
    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_eci_current_block_delta>();

    auto it_pair = idx.range(unbounded, boost::lambda::_1 > 0);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}


dbs_expertise_contribution::expertise_contributions_refs_type dbs_expertise_contribution::get_decreased_expertise_contributions_in_block() const
{
    expertise_contributions_refs_type ret;
    const auto& idx = db_impl()
      .get_index<expertise_contribution_index>()
      .indicies()
      .get<by_eci_current_block_delta>();

    auto it_pair = idx.range(unbounded, boost::lambda::_1 < 0);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}


} // namespace chain
} // namespace deip