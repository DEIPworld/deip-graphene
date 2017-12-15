#include <deip/chain/dbservice.hpp>
#include <chainbase/chainbase.hpp>

namespace deip {
namespace chain {

dbservice::dbservice(database& db)
    : _base_type(db)
{
}

dbservice::~dbservice()
{
}

// for TODO only:
chainbase::database& dbservice::_temporary_public_impl()
{
    return dynamic_cast<chainbase::database&>(*this);
}
}
}
