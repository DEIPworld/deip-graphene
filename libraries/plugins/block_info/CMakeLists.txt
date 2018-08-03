file(GLOB HEADERS "include/deip/plugins/block_info/*.hpp")

add_library( deip_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( deip_block_info deip_app deip_chain deip_protocol fc )
target_include_directories( deip_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_block_info_manifest SOURCES plugin.json)