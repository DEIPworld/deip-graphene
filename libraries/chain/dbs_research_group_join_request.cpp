#include <deip/chain/dbs_research_group_join_request.hpp>
#include <deip/chain/database.hpp>

namespace deip{
namespace chain{

dbs_research_group_join_request::dbs_research_group_join_request(database &db) : _base_type(db)
{
}

const research_group_join_request_object& dbs_research_group_join_request::create(const account_name_type& account_name,
                                                                                  const research_group_id_type& research_group_id,
                                                                                  const std::string motivation_letter)
{
    const auto& new_research_group_join_request = db_impl().create<research_group_join_request_object>([&](research_group_join_request_object& rgir_o) {
        rgir_o.account_name = account_name;
        rgir_o.research_group_id = research_group_id;
        fc::from_string(rgir_o.motivation_letter, motivation_letter);
        rgir_o.expiration_time = _get_now() + DAYS_TO_SECONDS(14);
    });

    return new_research_group_join_request;
}

const research_group_join_request_object& dbs_research_group_join_request::get(const research_group_join_request_id_type& research_group_join_request_id)
{
    return db_impl().get<research_group_join_request_object>(research_group_join_request_id);
}

const research_group_join_request_object&
    dbs_research_group_join_request::get_research_group_join_request_by_account_name_and_research_group_id(const account_name_type& account_name,
                                                                                                           const research_group_id_type& research_group_id)
{
    return db_impl().get<research_group_join_request_object, by_account_and_research_group_id>(boost::make_tuple(account_name, research_group_id));
}

void dbs_research_group_join_request::check_research_group_join_request_existence(const research_group_join_request_id_type& research_group_join_request_id)
{
    auto research_group_join_request = db_impl().find<research_group_join_request_object, by_id>(research_group_join_request_id);
    FC_ASSERT(research_group_join_request != nullptr, "Research group join request with id \"${1}\" must exist.", ("1", research_group_join_request_id));
}

dbs_research_group_join_request::research_group_join_request_refs_type
    dbs_research_group_join_request::get_research_group_join_requests_by_account_name(const account_name_type& account_name)
{
    research_group_join_request_refs_type ret;

    auto it_pair = db_impl().get_index<research_group_join_request_index>().indicies().get<by_account_name>().equal_range(account_name);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_research_group_join_request::research_group_join_request_refs_type
    dbs_research_group_join_request::get_research_group_join_requests_by_research_group_id(const research_group_id_type& research_group_id)
{
    research_group_join_request_refs_type ret;

    auto it_pair = db_impl().get_index<research_group_join_request_index>().indicies().get<by_research_group_id>().equal_range(research_group_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_research_group_join_request::clear_expired_research_group_join_requests()
{
    const auto& join_request_expiration_index = db_impl().get_index<research_group_join_request_index>().indices().get<by_expiration_time>();

    while (!join_request_expiration_index.empty() && is_expired(*join_request_expiration_index.begin()))
    {
        db_impl().remove(*join_request_expiration_index.begin());
    }
}

bool dbs_research_group_join_request::is_expired(const research_group_join_request_object& research_group_join_request)
{
    return _get_now() > research_group_join_request.expiration_time;
}



}
}