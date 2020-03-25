#pragma once

namespace deip {
namespace protocol {

struct eci_diff
{
    eci_diff(){}
    eci_diff(const share_type& _old, 
            const share_type& _new, 
            const fc::time_point_sec& timestamp,
            const uint16_t& alteration_source_type,
            const int64_t& alteration_source_id)
    : _old(_old)
    , _new(_new)
    , timestamp(timestamp)
    , alteration_source_type(alteration_source_type)
    , alteration_source_id(alteration_source_id)

    {
    }

    share_type _old;
    share_type _new;
    fc::time_point_sec timestamp;

    uint16_t alteration_source_type;
    int64_t alteration_source_id;

    share_type current() const
    {
        return _new;
    }

    share_type previous() const
    {
        return _old;
    }

    share_type diff() const
    {
        return _new - _old;
    }
};

} // namespace protocol
} // namespace deip

FC_REFLECT(deip::protocol::eci_diff, (_old)(_new)(timestamp)(alteration_source_type)(alteration_source_id))
