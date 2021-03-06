file(GLOB HEADERS "include/deip/protocol/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( deip_protocol

             types.cpp
             authority.cpp
             operations.cpp
             sign_state.cpp
             deip_operations.cpp
             operations/create_account_operation.cpp
             operations/update_account_operation.cpp
             operations/create_review_operation.cpp
             operations/create_grant_operation.cpp
             operations/create_award_operation.cpp
             operations/approve_award_operation.cpp
             operations/reject_award_operation.cpp
             operations/create_award_withdrawal_request_operation.cpp
             operations/certify_award_withdrawal_request_operation.cpp
             operations/approve_award_withdrawal_request_operation.cpp
             operations/reject_award_withdrawal_request_operation.cpp
             operations/pay_award_withdrawal_request_operation.cpp
             operations/create_research_nda_operation.cpp
             operations/sign_nda_contract_operation.cpp
             operations/decline_nda_contract_operation.cpp
             operations/close_nda_contract_operation.cpp
             operations/create_nda_content_access_request_operation.cpp
             operations/fulfill_nda_content_access_request_operation.cpp
             operations/join_research_contract_operation.cpp
             operations/leave_research_contract_operation.cpp
             operations/create_research_operation.cpp
             operations/create_research_content_operation.cpp
             operations/create_research_token_sale_operation.cpp
             operations/update_research_operation.cpp
             operations/create_proposal_operation.cpp
             operations/update_proposal_operation.cpp
             operations/delete_proposal_operation.cpp
             operations/transfer_research_share_operation.cpp
             operations/create_assessment_operation.cpp
             operations/create_research_license_operation.cpp
             operations/create_asset_operation.cpp
             operations/issue_asset_operation.cpp
             operations/reserve_asset_operation.cpp
             operations/create_contract_agreement.cpp
             operations/accept_contract_agreement.cpp
             operations/reject_contract_agreement_operation.cpp
             transaction.cpp
             block.cpp
             asset.cpp
             version.cpp
             percent.cpp
             get_config.cpp

             ${HEADERS}
           )

target_link_libraries( deip_protocol fc )
target_include_directories( deip_protocol
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   deip_protocol

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

INSTALL( FILES ${HEADERS} DESTINATION "include/deip/protocol" )
