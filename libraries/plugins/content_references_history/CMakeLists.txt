file(GLOB_RECURSE HEADERS "include/deip/content_references_history/*.hpp")

add_library( deip_cr_history
             cr_history_plugin.cpp
             applied_cr_operation.cpp
             cr_history_api.cpp
           )

target_link_libraries( deip_cr_history
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_cr_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_cr_history_manifest SOURCES plugin.json)

install( TARGETS
   deip_cr_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)