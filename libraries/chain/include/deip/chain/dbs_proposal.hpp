#pragma once

#include <deip/chain/dbs_base_impl.hpp>

namespace deip {
namespace chain {

using deip::protocol::asset;

class proposal_object;

class dbs_proposal : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_proposal(database& db);

public:

};

} // namespace chain
} // namespace deip