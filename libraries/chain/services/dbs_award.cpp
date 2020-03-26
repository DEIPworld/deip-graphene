#include <deip/chain/services/dbs_award.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_award::dbs_award(database &db)
        : _base_type(db)
{
}

const award_object& dbs_award::create_award(const funding_opportunity_id_type& funding_opportunity_id,
                                            const account_name_type &creator,
                                            const asset &amount)
{
    auto& new_award = db_impl().create<award_object>([&](award_object& award) {
        award.funding_opportunity_id = funding_opportunity_id;
        award.creator = creator;
        award.amount = amount;
    });

    return new_award;
}

const award_object& dbs_award::get_award(const award_id_type& id) const
{
    try{
        return db_impl().get<award_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const award_research_relation_object& dbs_award::create_award_research_relation(const award_id_type& award_id,
                                                                                const research_id_type& research_id,
                                                                                const research_group_id_type& research_group_id,
                                                                                const account_name_type& awardee,
                                                                                const asset& total_amount,
                                                                                const research_group_id_type& university_id,
                                                                                const share_type& university_overhead)
{
    auto& new_award_research_relation = db_impl().create<award_research_relation_object>([&](award_research_relation_object& arr_o) {
        arr_o.award_id = award_id;
        arr_o.research_id = research_id;
        arr_o.research_group_id = research_group_id;
        arr_o.awardee = awardee;
        arr_o.total_amount = total_amount;
        arr_o.total_expenses = asset(0, total_amount.symbol);
        arr_o.university_id = university_id;
        arr_o.university_overhead = university_overhead;
    });

    return new_award_research_relation;
}

const award_research_relation_object& dbs_award::get_award_research_relation(const award_research_relation_id_type& id)
{
    try{
        return db_impl().get<award_research_relation_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}


} //namespace chain
} //
