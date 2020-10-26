file(GLOB_RECURSE HEADERS "include/deip/investments_history/*.hpp")

add_library( deip_investments_history
             investments_history_plugin.cpp
             investments_history_api.cpp
           )

target_link_libraries( deip_investments_history
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_investments_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_investments_history_manifest SOURCES plugin.json)

install( TARGETS
   deip_investments_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)