#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

enum contract_status : uint16_t
{
    contract_pending = 1,
    contract_signed = 2,
    contract_declined = 3, // by signee
    contract_closed = 4, // by creator
    contract_expired = 5
};

class contract_object : public object<contract_object_type, contract_object>
{
    contract_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    contract_object(Constructor&& c, allocator<Allocator> a) : title(a), creator_signature(a), signee_signature(a), contract_hash(a)
    {
        c(*this);
    }

    contract_id_type id;
    fc::shared_string title;

    account_name_type creator;
    account_name_type signee;

    research_group_id_type creator_research_group_id;
    research_group_id_type signee_research_group_id;

    fc::shared_string creator_signature;
    fc::shared_string signee_signature;

    fc::shared_string contract_hash;
    contract_status status = contract_status::contract_pending;

    fc::time_point_sec created_at;
    fc::time_point_sec start_date;
    fc::time_point_sec end_date;
};

struct by_creator;
struct by_signee;
struct by_contract_hash;
struct by_end_date;
struct by_creator_research_group;
struct by_signee_research_group;
struct by_creator_research_group_and_contract_hash;
struct by_signee_research_group_and_contract_hash;
struct by_creator_research_group_and_signee_research_group_and_contract_hash;

typedef multi_index_container<contract_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<contract_object,
                                                                contract_id_type,
                                                               &contract_object::id>>,
                                         ordered_non_unique<tag<by_creator>,
                                                        member<contract_object,
                                                                account_name_type,
                                                               &contract_object::creator>>,
                                         ordered_non_unique<tag<by_signee>,
                                                        member<contract_object,
                                                                account_name_type,
                                                               &contract_object::signee>>,
                                         ordered_non_unique<tag<by_contract_hash>,
                                                        member<contract_object,
                                                                fc::shared_string,
                                                               &contract_object::contract_hash>>,
                                         ordered_non_unique<tag<by_end_date>,
                                                        member<contract_object,
                                                                fc::time_point_sec,
                                                                &contract_object::end_date>>,
                                         ordered_non_unique<tag<by_creator_research_group>,
                                                        member<contract_object,
                                                                research_group_id_type,
                                                                &contract_object::creator_research_group_id>>,
                                         ordered_non_unique<tag<by_signee_research_group>,
                                                        member<contract_object,
                                                                research_group_id_type,
                                                                &contract_object::signee_research_group_id>>, 
                                         ordered_non_unique<tag<by_creator_research_group_and_contract_hash>,
                                                composite_key<contract_object,
                                                        member<contract_object,
                                                                research_group_id_type,
                                                                &contract_object::creator_research_group_id>,
                                                        member<contract_object,
                                                                fc::shared_string,
                                                                &contract_object::contract_hash>>,
                                                composite_key_compare<std::less<research_group_id_type>,
                                                        fc::strcmp_less>>,
                                         ordered_non_unique<tag<by_signee_research_group_and_contract_hash>,
                                                composite_key<contract_object,
                                                        member<contract_object,
                                                                research_group_id_type,
                                                                &contract_object::signee_research_group_id>,
                                                        member<contract_object,
                                                                fc::shared_string,
                                                                &contract_object::contract_hash>>,
                                                composite_key_compare<std::less<research_group_id_type>,
                                                        fc::strcmp_less>>,
                                         ordered_non_unique<tag<by_creator_research_group_and_signee_research_group_and_contract_hash>,
                                                composite_key<contract_object,
                                                        member<contract_object,
                                                                research_group_id_type,
                                                                &contract_object::creator_research_group_id>,
                                                        member<contract_object,
                                                                research_group_id_type,
                                                                &contract_object::signee_research_group_id>,
                                                        member<contract_object,
                                                                fc::shared_string,
                                                                &contract_object::contract_hash>>,
                                                composite_key_compare<
                                                        std::less<research_group_id_type>,
                                                        std::less<research_group_id_type>,
                                                        fc::strcmp_less>>>,

                              allocator<contract_object>>
    contract_index;

}
}

FC_REFLECT_ENUM(deip::chain::contract_status, (contract_pending)(contract_signed)(contract_declined)(contract_closed)(contract_expired))

FC_REFLECT( deip::chain::contract_object,
             (id)(title)(creator)(signee)(creator_research_group_id)(signee_research_group_id)(creator_signature)(signee_signature)(contract_hash)(status)(created_at)(start_date)(end_date)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::contract_object, deip::chain::contract_index )

