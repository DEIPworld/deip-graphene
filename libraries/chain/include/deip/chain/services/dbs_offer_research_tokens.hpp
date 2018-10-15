#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/offer_research_tokens_object.hpp>

#include <vector>

namespace deip{
namespace chain{

class dbs_offer_research_tokens : public dbs_base{

    friend class dbservice_dbs_factory;

    dbs_offer_research_tokens() = delete;

protected:

    explicit dbs_offer_research_tokens(database &db);

public:

    using offer_research_tokens_refs_type = std::vector<std::reference_wrapper<const offer_research_tokens_object>>;

    const offer_research_tokens_object& create(const account_name_type& sender,
                                               const account_name_type& receiver,
                                               const research_id_type& research_id,
                                               const share_type amount,
                                               const asset& price);

    offer_research_tokens_refs_type get_offers() const;

    const offer_research_tokens_object& get(const offer_research_tokens_id_type& id) const;

    offer_research_tokens_refs_type get_offers_by_receiver(const account_name_type& receiver) const;

    const offer_research_tokens_object& get_offer_by_receiver_and_research_id(const account_name_type& receiver,
                                                                              const research_id_type& research_id) const;

    offer_research_tokens_refs_type get_offers_by_research_id(const research_id_type& research_id) const;

    void check_offer_existence(const offer_research_tokens_id_type& id) const;
};
}
}