#include <deip/chain/services/dbs_offer_research_tokens.hpp>
#include <deip/chain/database/database.hpp>


namespace deip{
namespace chain{

dbs_offer_research_tokens::dbs_offer_research_tokens(database &db) : _base_type(db)
{
}

const offer_research_tokens_object& dbs_offer_research_tokens::create(const account_name_type& sender,
                                                                      const account_name_type& receiver,
                                                                      const research_id_type& research_id,
                                                                      const share_type amount,
                                                                      const asset& price)
{
    const auto& new_offer = db_impl().create<offer_research_tokens_object>([&](offer_research_tokens_object& o) {
        o.sender = sender;
        o.receiver = receiver;
        o.research_id = research_id;
        o.amount = amount;
        o.price = price;
    });

    return new_offer;
}

dbs_offer_research_tokens::offer_research_tokens_refs_type dbs_offer_research_tokens::get_offers() const
{
    offer_research_tokens_refs_type ret;

    const auto& idx = db_impl().get_index<offer_research_tokens_index>().indicies().get<by_id>();
    auto it = idx.lower_bound(0);
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const offer_research_tokens_object& dbs_offer_research_tokens::get(const offer_research_tokens_id_type& id) const
{
    try {
        return db_impl().get<offer_research_tokens_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

dbs_offer_research_tokens::offer_research_tokens_refs_type dbs_offer_research_tokens::get_offers_by_receiver(const account_name_type& receiver) const
{
    offer_research_tokens_refs_type ret;

    auto it_pair = db_impl().get_index<offer_research_tokens_index>().indicies().get<by_receiver>().equal_range(receiver);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const offer_research_tokens_object& dbs_offer_research_tokens::get_offer_by_receiver_and_research_id(const account_name_type& receiver,
                                                                                                     const research_id_type& research_id) const
{
    try {
        return db_impl().get<offer_research_tokens_object, by_receiver_and_research_id>(std::make_tuple(receiver, research_id));
    }
    FC_CAPTURE_AND_RETHROW((receiver)(research_id))
}

dbs_offer_research_tokens::offer_research_tokens_refs_type dbs_offer_research_tokens::get_offers_by_research_id(const research_id_type& research_id) const
{
    offer_research_tokens_refs_type ret;

    auto it_pair = db_impl().get_index<offer_research_tokens_index>().indicies().get<by_research_id>().equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_offer_research_tokens::check_offer_existence(const offer_research_tokens_id_type& id) const
{
    auto offer = db_impl().find<offer_research_tokens_object, by_id>(id);
    FC_ASSERT(offer != nullptr, "Offer with id \"${1}\" must exist.", ("1", id));
}

}
}