#pragma once

#include <deip/app/deip_api_objects.hpp>
#include <deip/fo_history/withdrawal_request_history_object.hpp>

namespace deip {
namespace fo_history {

struct withdrawal_request_history_api_obj
{
    withdrawal_request_history_api_obj(const withdrawal_request_history_object& wrh_o)
        : id(wrh_o.id._id)
        , payment_number(fc::to_string(wrh_o.payment_number))
        , award_number(fc::to_string(wrh_o.award_number))
        , subaward_number(fc::to_string(wrh_o.subaward_number))
        , requester(wrh_o.requester)
        , certifier(wrh_o.certifier)
        , approver(wrh_o.approver)
        , rejector(wrh_o.rejector)
        , payer(wrh_o.payer)
        , amount(wrh_o.amount)
        , description(fc::to_string(wrh_o.description))
        , attachment(fc::to_string(wrh_o.attachment))
        , status(wrh_o.status)
        , version(wrh_o.version)
        , trx_id(wrh_o.trx_id)
        , block(wrh_o.block)
        , trx_in_block(wrh_o.trx_in_block)
        , op_in_trx(wrh_o.op_in_trx)
    {
    }

    withdrawal_request_history_api_obj()
    {
    }

    int64_t id;

    string payment_number;
    string award_number;
    string subaward_number;

    account_name_type requester;
    account_name_type certifier;
    account_name_type approver;
    account_name_type rejector;
    account_name_type payer;

    asset amount;

    string description;
    string attachment;

    uint16_t status;
    uint16_t version;

    transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;

};


}
}

FC_REFLECT(deip::fo_history::withdrawal_request_history_api_obj,
    (id)
    (payment_number)
    (award_number)
    (subaward_number)
    (requester)
    (certifier)
    (approver)
    (rejector)
    (payer)
    (amount)
    (description)
    (attachment)
    (status)
    (version)
    (trx_id)
    (block)
    (trx_in_block)
    (op_in_trx)
)