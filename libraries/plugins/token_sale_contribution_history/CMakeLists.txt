file(GLOB_RECURSE HEADERS "include/deip/token_sale_contribution_history/*.hpp")

add_library( deip_tsc_history
             tsc_history_plugin.cpp
             applied_tsc_operation.cpp
             tsc_history_api.cpp
           )

target_link_libraries( deip_tsc_history
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_tsc_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_tsc_history_manifest SOURCES plugin.json)

install( TARGETS
   deip_tsc_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)