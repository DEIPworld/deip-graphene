file(GLOB_RECURSE HEADERS "include/deip/proposal_history/*.hpp")

add_library( deip_proposal_history
             proposal_history_plugin.cpp
             proposal_history_api.cpp
           )

target_link_libraries( deip_proposal_history
                       deip_account_by_key
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_proposal_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_proposal_history_manifest SOURCES plugin.json)

install( TARGETS
   deip_proposal_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)