file(GLOB_RECURSE HEADERS "include/deip/blockchain_history/*.hpp")
 add_library( deip_blockchain_history
             blockchain_history_plugin.cpp
             account_history_api.cpp
             blockchain_history_api.cpp
             applied_operation.cpp
           )
 target_link_libraries( deip_blockchain_history
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_blockchain_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
 add_custom_target( deip_blockchain_history_manifest SOURCES plugin.json)
 install( TARGETS
   deip_blockchain_history
    RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)