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
             database.cpp
             fork_database.cpp
             database_witness_schedule.cpp

             dbs_base_impl.cpp
             dbservice.cpp
             dbs_account.cpp
             dbs_reward.cpp
             dbs_witness.cpp
             dbs_budget.cpp
             dbs_proposal.cpp
             dbs_research_group.cpp
             dbs_discipline.cpp
             dbs_research.cpp
             dbs_research_discipline_relation.cpp
             dbs_research_content.cpp
             dbs_expert_token.cpp


             deip_evaluator.cpp
             deip_objects.cpp
             shared_authority.cpp
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
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
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
