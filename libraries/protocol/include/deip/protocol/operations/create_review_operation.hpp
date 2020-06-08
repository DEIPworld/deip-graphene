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

    FIRST = novelty,
    LAST = replication
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
    std::string content;
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

#define DECLARE_ASSESSMENT_MODEL_TYPE(AssessmentModelType)                                                             \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const AssessmentModelType&, fc::variant&);                                                         \
    void from_variant(const fc::variant&, AssessmentModelType&);                                                       \
                                                                                                                       \
    } /* fc */


namespace fc {
using namespace deip::protocol;

std::string assessment_model_name_from_type(const std::string& type_name);

struct from_assessment_model_type
{
    variant& var;
    from_assessment_model_type(variant& dv)
        : var(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        auto name = assessment_model_name_from_type(fc::get_typename<T>::name());
        var = variant(std::make_pair(name, v));
    }
};

struct get_assessment_model_type
{
  string& name;
    get_assessment_model_type(string& dv)
    : name(dv)
  {
  }

  typedef void result_type;
  template <typename T> void operator()(const T& v) const
  {
    name = assessment_model_name_from_type(fc::get_typename<T>::name());
  }
};
} // namespace fc



#define DEFINE_ASSESSMENT_MODELS_TYPE(AssessmentModelType)                                                             \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const AssessmentModelType& var, fc::variant& vo)                                                   \
    {                                                                                                                  \
        var.visit(from_assessment_model_type(vo));                                                                     \
    }                                                                                                                  \
                                                                                                                       \
    void from_variant(const fc::variant& var, AssessmentModelType& vo)                                                 \
    {                                                                                                                  \
        static std::map<string, uint32_t> to_tag = []() {                                                              \
            std::map<string, uint32_t> name_map;                                                                       \
            for (int i = 0; i < AssessmentModelType::count(); ++i)                                                     \
            {                                                                                                          \
                AssessmentModelType tmp;                                                                               \
                tmp.set_which(i);                                                                                      \
                string n;                                                                                              \
                tmp.visit(get_assessment_model_type(n));                                                               \
                name_map[n] = i;                                                                                       \
            }                                                                                                          \
            return name_map;                                                                                           \
        }();                                                                                                           \
                                                                                                                       \
        auto ar = var.get_array();                                                                                     \
        if (ar.size() < 2)                                                                                             \
            return;                                                                                                    \
        if (ar[0].is_uint64())                                                                                         \
            vo.set_which(ar[0].as_uint64());                                                                           \
        else                                                                                                           \
        {                                                                                                              \
            auto itr = to_tag.find(ar[0].as_string());                                                                 \
            FC_ASSERT(itr != to_tag.end(), "Invalid assessment model type: ${n}", ("n", ar[0]));                       \
            vo.set_which(to_tag[ar[0].as_string()]);                                                                   \
        }                                                                                                              \
        vo.visit(fc::to_static_variant(ar[1]));                                                                        \
    }                                                                                                                  \
    } /* fc */

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
)



DECLARE_ASSESSMENT_MODEL_TYPE(deip::protocol::assessment_models)
FC_REFLECT_TYPENAME(deip::protocol::assessment_models)