#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/version.hpp>

namespace deip {
namespace protocol {

struct base_assessment_model
{
    std::string version;

    base_assessment_model(string v = "1.0.0")
    {
        version = v;
    }

    deip::protocol::version get_version() const
    {
        fc::variant v_str(version);
        deip::protocol::version v;
        fc::from_variant(v_str, v);
        return v;
    }
};

struct binary_scoring_assessment_model_v1_0_0_type : base_assessment_model
{
    binary_scoring_assessment_model_v1_0_0_type(string v = "1.0.0")
        : base_assessment_model(v)
    {
    }

    bool is_positive;
};

struct multicriteria_scoring_assessment_model_v1_0_0_type : base_assessment_model
{
    multicriteria_scoring_assessment_model_v1_0_0_type(string v = "1.0.0")
        : base_assessment_model(v)
    {
    }

    std::map<uint16_t, uint16_t> scores;
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
        binary_scoring_assessment_model_v1_0_0_type,
        multicriteria_scoring_assessment_model_v1_0_0_type
        >
        assessment_models;

struct make_review_operation : public base_operation
{
    account_name_type author;
    int64_t research_content_id;
    std::string content;
    uint16_t weight;
    assessment_models assessment_model;
    extensions_type extensions;

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


FC_REFLECT( deip::protocol::make_review_operation, (author)(research_content_id)(content)(weight)(assessment_model)(extensions) )

FC_REFLECT( deip::protocol::base_assessment_model, (version) )
FC_REFLECT_DERIVED( deip::protocol::binary_scoring_assessment_model_v1_0_0_type, (deip::protocol::base_assessment_model), (is_positive) )
FC_REFLECT_DERIVED( deip::protocol::multicriteria_scoring_assessment_model_v1_0_0_type, (deip::protocol::base_assessment_model), (scores) )

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