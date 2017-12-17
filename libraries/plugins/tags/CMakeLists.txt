file(GLOB HEADERS "include/deip/tags/*.hpp")

add_library( deip_tags
             tags_plugin.cpp )

target_link_libraries( deip_tags deip_chain deip_protocol deip_app )
target_include_directories( deip_tags
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   deip_tags

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
