#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/version.hpp>

namespace deip {
namespace protocol {

using deip::protocol::external_id_type;

struct binary_scoring_assessment_model_type
{
    bool is_positive;
    extensions_type extensions;
};

struct multicriteria_scoring_assessment_model_type
{
    flat_map<uint16_t, uint16_t> scores;
    extensions_type extensions;
};

enum class assessment_criteria : uint16_t
{
    unknown = 0,
    novelty = 1,
    technical_quality = 2,
    methodology = 3,
    impact = 4,
    rationality = 5,
    replication = 6,
    commercialization = 7,

    FIRST = novelty,
    LAST = commercialization
};

typedef fc::static_variant<
    binary_scoring_assessment_model_type,
    multicriteria_scoring_assessment_model_type
    >
    assessment_models;

struct create_review_operation : public entity_operation
{
    external_id_type external_id;
    account_name_type author;
    external_id_type research_content_external_id;
    string content;
    percent weight;
    assessment_models assessment_model;
    flat_set<external_id_type> disciplines;
    extensions_type extensions;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(author);
    }
};

}
}

FC_REFLECT(deip::protocol::create_review_operation,
           (external_id)
           (author)
           (research_content_external_id)
           (content)
           (weight)
           (assessment_model)
           (disciplines)
           (extensions))

FC_REFLECT( deip::protocol::binary_scoring_assessment_model_type, (is_positive)(extensions) )
FC_REFLECT( deip::protocol::multicriteria_scoring_assessment_model_type, (scores)(extensions) )

FC_REFLECT_ENUM( deip::protocol::assessment_criteria, (unknown)
                                                      (novelty)
                                                      (technical_quality)
                                                      (methodology)
                                                      (impact)
                                                      (rationality)
                                                      (replication)
                                                      (commercialization)
)

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::assessment_models)
FC_REFLECT_TYPENAME(deip::protocol::assessment_models)