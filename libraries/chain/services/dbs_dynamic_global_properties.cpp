#include <deip/chain/services/dbs_dynamic_global_properties.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_dynamic_global_properties::dbs_dynamic_global_properties(database& db)
    : _base_type(db)
{
}

const dynamic_global_property_object& dbs_dynamic_global_properties::get_dynamic_global_properties() const
{
    try {
        return db_impl().get_dynamic_global_properties();
    }
    FC_CAPTURE_AND_RETHROW();
}

const recent_entity_object& dbs_dynamic_global_properties::create_recent_entity(const external_id_type& external_id)
{
    const auto& block_time = db_impl().head_block_time();

    const uint16_t ref_block_num = db_impl().current_proposed_trx().valid()
        ? db_impl().current_proposed_trx()->ref_block_num
        : db_impl().current_trx_ref_block_num();

    const uint32_t ref_block_prefix = db_impl().current_proposed_trx().valid()
      ? db_impl().current_proposed_trx()->ref_block_prefix
      : db_impl().current_trx_ref_block_prefix();

    const recent_entity_object& recent_entity = db_impl().create<recent_entity_object>([&](recent_entity_object& e_o) {
        e_o.external_id = external_id;
        e_o.ref_block_num = ref_block_num;
        e_o.ref_block_prefix = ref_block_prefix;
        e_o.created_at = block_time;
    });

    return recent_entity;
}

void dbs_dynamic_global_properties::clear_expired_recent_entities(const signed_block& next_block)
{
    const uint16_t ref_block_num = next_block.block_num() & 0xffff;
    const uint32_t ref_block_prefix = next_block.id()._hash[1];
    const auto& block_time = db_impl().head_block_time();

    const auto& idx = db_impl()
      .get_index<recent_entity_index>()
      .indicies()
      .get<by_block_number>();

    const auto& itr_pair = idx.equal_range(ref_block_num);

    auto itr = itr_pair.first;
    const auto itr_end = itr_pair.second;

    while (itr != itr_end)
    {
        auto entity = itr++;
        if (entity->ref_block_prefix != ref_block_prefix && 
            entity->created_at <= block_time - DEIP_RECENT_ENTITY_LIFETIME_SEC)
        {
            db_impl().remove(*entity);
        }
    }
}

const bool dbs_dynamic_global_properties::entity_exists(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<recent_entity_index>()
      .indicies()
      .get<by_external_id>();

    return idx.find(external_id) != idx.end();
}

} // namespace chain
} // namespace deip