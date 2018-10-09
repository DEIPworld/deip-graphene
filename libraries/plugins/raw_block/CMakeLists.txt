file(GLOB HEADERS "include/deip/plugins/raw_block/*.hpp")

add_library( deip_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( deip_raw_block deip_app deip_chain deip_protocol fc )
target_include_directories( deip_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_raw_block_manifest SOURCES plugin.json)