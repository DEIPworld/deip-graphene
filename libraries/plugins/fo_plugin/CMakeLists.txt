file(GLOB_RECURSE HEADERS "include/deip/fo_history/*.hpp")

add_library( deip_fo_history
             fo_history_plugin.cpp
             fo_history_api.cpp
           )

target_link_libraries( deip_fo_history
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_fo_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_fo_history_manifest SOURCES plugin.json)

install( TARGETS
   deip_fo_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
