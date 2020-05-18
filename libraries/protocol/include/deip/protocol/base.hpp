#pragma once

#include <deip/protocol/types.hpp>
#include <deip/protocol/authority.hpp>
#include <deip/protocol/version.hpp>

#include <fc/time.hpp>

namespace deip {
namespace protocol {

struct base_operation
{
    void get_required_authorities(vector<authority>&) const {}
    void get_required_active_authorities(flat_set<account_name_type>&) const {}
    void get_required_posting_authorities(flat_set<account_name_type>&) const {}
    void get_required_owner_authorities(flat_set<account_name_type>&) const {}

    bool is_virtual() const { return false; }
    void validate() const {}

    std::string entity_id() const { return std::string(); }
    external_id_type get_entity_id() const { return std::string(); }
    bool ignore_entity_id_validation() const { return false; }

    template <typename T>
    void validate_entity_id(const T& entity, uint16_t ref_block_num, uint32_t ref_block_prefix) const {}
};

struct virtual_operation : public base_operation
{
    bool is_virtual() const { return true; }
    void validate() const { FC_ASSERT(false, "This is a virtual operation"); }
};


template <typename Class> struct entity_id_extractor
{
    entity_id_extractor(const Class& op,
                        const std::string& name,
                        std::vector<char>& extracted,
                        std::vector<char>& provided)
        : entity(op)
        , entity_id_field_name(name)
        , extracted_entity_id(extracted)
        , provided_entity_id(provided)

    {
    }

    template <typename T, typename C, T(C::*p)> void operator()(const char* name) const
    {
        if (entity_id_field_name != (std::string)name)
        {
            const std::vector<char> data = fc::raw::pack(entity.*p);
            extracted_entity_id.insert(extracted_entity_id.end(), data.begin(), data.end());
        }
        else 
        {
            const std::vector<char> data = fc::raw::pack(entity.*p);
            const std::string val = fc::raw::unpack<std::string>(data);
            provided_entity_id.insert(provided_entity_id.end(), val.begin(), val.end());
        }
    }

private:
    const Class& entity;
    const std::string entity_id_field_name;
    std::vector<char>& extracted_entity_id;
    std::vector<char>& provided_entity_id;
};


struct entity_operation : public base_operation
{

    template <typename T>
    void validate_entity_id(const T& entity, uint16_t ref_block_num, uint32_t ref_block_prefix) const
    {
        const std::string entity_id_field = entity.entity_id();

        std::vector<char> provided_id;
        std::vector<char> extracted_id;
        const std::vector<char> ref_block_num_data = fc::raw::pack(ref_block_num);
        const std::vector<char> ref_block_prefix_data = fc::raw::pack(ref_block_prefix);
        extracted_id.insert(extracted_id.end(), ref_block_num_data.begin(), ref_block_num_data.end());
        extracted_id.insert(extracted_id.end(), ref_block_prefix_data.begin(), ref_block_prefix_data.end());

        fc::reflector<T>::visit(entity_id_extractor<T>(entity, entity_id_field, extracted_id, provided_id));

        const std::string extracted = (std::string)fc::ripemd160::hash(extracted_id.data(), (uint32_t)extracted_id.size());
        const std::string provided = std::string(provided_id.begin(), provided_id.end());
        const std::string specified = entity.get_entity_id();

        FC_ASSERT(provided == extracted, "Provided entity id (${1}) does not match the extracted entity id (${2})", ("1", provided)("2", extracted));
        FC_ASSERT(specified == extracted, "Specified entity id (${1}) does not match the extracted entity id (${2})", ("1", specified)("2", extracted));
    }
};


typedef static_variant<void_t,
    version, // Normal witness version reporting, for diagnostics and voting
    hardfork_version_vote // Voting for the next hardfork to trigger
    >
    block_header_extensions;

typedef static_variant<void_t> future_extensions;

typedef flat_set<block_header_extensions> block_header_extensions_type;
typedef flat_set<future_extensions> extensions_type;
}
} // deip::protocol

FC_REFLECT_TYPENAME(deip::protocol::block_header_extensions)
FC_REFLECT_TYPENAME(deip::protocol::future_extensions)
