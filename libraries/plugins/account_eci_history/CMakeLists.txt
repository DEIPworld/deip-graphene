file(GLOB_RECURSE HEADERS "include/deip/account_eci_history/*.hpp")

add_library( deip_account_eci_history
             account_eci_history_plugin.cpp
             applied_account_eci_operation.cpp
             account_eci_history_api.cpp
           )

target_link_libraries( deip_account_eci_history
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_account_eci_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_account_eci_history_manifest SOURCES plugin.json)

install( TARGETS
   deip_account_eci_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
