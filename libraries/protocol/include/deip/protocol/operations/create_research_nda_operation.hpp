#pragma once
#include <deip/protocol/base.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

using deip::protocol::external_id_type;

struct create_research_nda_operation : public entity_operation
{
    external_id_type external_id;
    account_name_type creator;
    std::set<account_name_type> parties;
    string description;
    external_id_type research_external_id;
    optional<fc::time_point_sec> start_time;
    fc::time_point_sec end_time;

    extensions_type extensions;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
        for (const account_name_type& party : parties)
        {
            a.insert(party);
        }
    }
};

}
}

FC_REFLECT( deip::protocol::create_research_nda_operation,
    (external_id)
    (creator)
    (parties)
    (description)
    (research_external_id)
    (start_time)
    (end_time)
    (extensions)
)
