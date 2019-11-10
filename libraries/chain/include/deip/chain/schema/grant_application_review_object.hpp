#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"
#include "shared_authority.hpp"

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

#include <vector>

namespace deip {
namespace chain {

class grant_application_review_object : public object<grant_application_review_object_type, grant_application_review_object>
{

    grant_application_review_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    grant_application_review_object(Constructor&& c, allocator<Allocator> a)
        : content(a)
        , disciplines(a)
        , references(a)
    {
        c(*this);
    }

    grant_application_review_id_type id;
    grant_application_id_type grant_application_id;
    fc::shared_string content;
    bool is_positive = true;
    account_name_type author;
    time_point_sec created_at;
    discipline_id_type_set disciplines;
    research_content_id_type_set references;
};

struct by_author;
struct by_grant_application;
struct by_author_and_grant_application;

typedef multi_index_container<
    grant_application_review_object,
    indexed_by<
        ordered_unique<tag<by_id>,
                       member<grant_application_review_object,
                              grant_application_review_id_type,
                              &grant_application_review_object::id>>,
        ordered_unique<
            tag<by_author_and_grant_application>,
            composite_key<
                grant_application_review_object,
                member<grant_application_review_object, account_name_type, &grant_application_review_object::author>,
                member<grant_application_review_object,
                       grant_application_id_type,
                       &grant_application_review_object::grant_application_id>>>,
        ordered_non_unique<
            tag<by_author>,
            member<grant_application_review_object, account_name_type, &grant_application_review_object::author>>,
        ordered_non_unique<tag<by_grant_application>,
                           member<grant_application_review_object,
                                  grant_application_id_type,
                                  &grant_application_review_object::grant_application_id>>>,
    allocator<grant_application_review_object>>
    grant_application_review_index;
} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::grant_application_review_object,
           (id)(grant_application_id)(content)(is_positive)(author)(created_at)(disciplines)(references))

CHAINBASE_SET_INDEX_TYPE(deip::chain::grant_application_review_object, deip::chain::grant_application_review_index)