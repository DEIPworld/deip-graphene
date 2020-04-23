file(GLOB HEADERS "include/deip/chain/*.hpp" "include/deip/chain/util/*.hpp")

if( MSVC )
   set( hardfork_hpp_file "${CMAKE_CURRENT_SOURCE_DIR}/include/deip/chain/hardfork.hpp" )
   add_custom_target( build_hardfork_hpp
      COMMAND cat-parts "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file} )
   add_dependencies( build_hardfork_hpp cat-parts )
else( MSVC )
   set( hardfork_hpp_file "${CMAKE_CURRENT_BINARY_DIR}/include/deip/chain/hardfork.hpp")
   add_custom_target( build_hardfork_hpp
      COMMAND "${CMAKE_SOURCE_DIR}/programs/build_helpers/cat_parts.py" "${CMAKE_CURRENT_SOURCE_DIR}/hardfork.d" ${hardfork_hpp_file} )
endif( MSVC )


set_source_files_properties( "${CMAKE_CURRENT_BINARY_DIR}/include/deip/chain/hardfork.hpp" PROPERTIES GENERATED TRUE )

## SORT .cpp by most likely to change / break compile
add_library( deip_chain

             # As database takes the longest to compile, start it first
        database/database.cpp
        database/fork_database.cpp
        database/database_witness_schedule.cpp

        services/dbs_base_impl.cpp
        dbservice.cpp
        services/dbs_account.cpp
        services/dbs_witness.cpp
        services/dbs_discipline_supply.cpp
        services/dbs_proposal.cpp
        services/dbs_research_group.cpp
        services/dbs_discipline.cpp
        services/dbs_review_vote.cpp
        services/dbs_expertise_contribution.cpp
        services/dbs_research.cpp
        services/dbs_research_discipline_relation.cpp
        services/dbs_research_content.cpp
        services/dbs_expert_token.cpp
        services/dbs_research_token.cpp
        services/dbs_research_token_sale.cpp
        services/dbs_research_group_invite.cpp
        services/dbs_dynamic_global_properties.cpp
        services/dbs_review.cpp
        services/dbs_vesting_balance.cpp
        services/dbs_reward_pool.cpp
        services/dbs_expertise_allocation_proposal.cpp
        services/dbs_grant.cpp
        services/dbs_grant_application.cpp
        services/dbs_grant_application_review.cpp
        services/dbs_funding_opportunity.cpp
        services/dbs_account_balance.cpp
        services/dbs_asset.cpp
        services/dbs_award.cpp
        services/dbs_nda_contract.cpp
        services/dbs_nda_contract_requests.cpp

        deip_evaluator.cpp
        schema/deip_objects.cpp
        schema/shared_authority.cpp
        schema/proposal_object.cpp
        block_log.cpp

        genesis.cpp

        util/reward.cpp
             
        ${HEADERS}
        ${hardfork_hpp_file}
        "${CMAKE_CURRENT_BINARY_DIR}/include/deip/chain/hardfork.hpp"
        )

add_dependencies( deip_chain deip_protocol build_hardfork_hpp )
target_link_libraries( deip_chain deip_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( deip_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties(database/database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

if(UNIX)
  set_source_files_properties( deip_evaluator.cpp PROPERTIES COMPILE_FLAGS "-Wno-sign-compare -Wno-maybe-uninitialized" )
endif(UNIX)

INSTALL( TARGETS
   deip_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/deip/chain" )
