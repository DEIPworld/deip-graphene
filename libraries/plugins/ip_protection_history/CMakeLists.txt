file(GLOB_RECURSE HEADERS "include/deip/ip_protection_history/*.hpp")

add_library( deip_ip_protection_history
             ip_protection_history_plugin.cpp
             applied_ip_protection_operation.cpp
             ip_protection_history_api.cpp
        )

target_link_libraries( deip_ip_protection_history
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_ip_protection_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_ip_protection_history_manifest SOURCES plugin.json)

install( TARGETS
   deip_ip_protection_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
