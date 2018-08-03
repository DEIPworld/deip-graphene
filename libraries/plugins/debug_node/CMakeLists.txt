file(GLOB HEADERS "include/deip/plugins/debug_node/*.hpp")

add_library( deip_debug_node
             ${HEADERS}
             debug_node_plugin.cpp
             debug_node_api.cpp
           )

target_link_libraries( deip_debug_node deip_app deip_chain deip_protocol fc )
target_include_directories( deip_debug_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_debug_node_manifest SOURCES plugin.json)

install( TARGETS deip_debug_node
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib
       )
INSTALL( FILES ${HEADERS} DESTINATION "include/deip/plugins/debug_node" )
