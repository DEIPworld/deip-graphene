#pragma once

#include <deip/ip_protection_history/ip_protection_operation_object.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace ip_protection_history {

template <uint16_t IpProtectionHistoryType> struct ip_protection_history_object : public object<IpProtectionHistoryType, ip_protection_history_object<IpProtectionHistoryType>>
{
public:
    template <typename Constructor, typename Allocator> ip_protection_history_object(Constructor&& c, allocator<Allocator> a) : content_hash(a)
    {
        c(*this);
    }

    typedef typename object<IpProtectionHistoryType, ip_protection_history_object<IpProtectionHistoryType>>::id_type id_type;

    id_type id;

    research_id_type research_id;
    fc::shared_string content_hash;
    ip_protection_operation_object::id_type op;
};

struct by_content_hash;
struct by_research_and_hash;

template <typename ip_protection_history_object_t>
using ip_protection_history_index
    = chainbase::shared_multi_index_container<ip_protection_history_object_t,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<ip_protection_history_object_t,
                                                                    typename ip_protection_history_object_t::id_type,
                                                                    &ip_protection_history_object_t::id>>,

                                              ordered_non_unique<tag<by_content_hash>,
                                                             member<ip_protection_history_object_t,
                                                                    fc::shared_string,
                                                                    &ip_protection_history_object_t::content_hash>>,

                                              ordered_unique<tag<by_research_and_hash>,
                                              composite_key<ip_protection_history_object_t,
                                                            member<ip_protection_history_object_t,
                                                                   research_id_type,
                                                                   &ip_protection_history_object_t::research_id>,
                                                            member<ip_protection_history_object_t,
                                                                   fc::shared_string,
                                                                   &ip_protection_history_object_t::content_hash>>,
                                              composite_key_compare<std::less<research_id_type>,
                                                                              fc::strcmp_less>>>>;

using all_ip_protection_operations_history_object = ip_protection_history_object<all_ip_protection_operations_history>;
using create_research_material_history_object = ip_protection_history_object<create_research_materials_history>;

using ip_protection_operations_full_history_index = ip_protection_history_index<all_ip_protection_operations_history_object>;
using create_research_material_history_index = ip_protection_history_index<create_research_material_history_object>;

} // namespace ip_protection_history
} // namespace deip

FC_REFLECT(deip::ip_protection_history::all_ip_protection_operations_history_object, (id)(research_id)(content_hash)(op))
FC_REFLECT(deip::ip_protection_history::create_research_material_history_object, (id)(research_id)(content_hash)(op))

CHAINBASE_SET_INDEX_TYPE(deip::ip_protection_history::all_ip_protection_operations_history_object,
                         deip::ip_protection_history::ip_protection_operations_full_history_index)

CHAINBASE_SET_INDEX_TYPE(deip::ip_protection_history::create_research_material_history_object,
                         deip::ip_protection_history::create_research_material_history_index)
