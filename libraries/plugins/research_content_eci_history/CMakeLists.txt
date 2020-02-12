file(GLOB_RECURSE HEADERS "include/deip/research_content_eci_history/*.hpp")

add_library( deip_research_content_eci_history
             research_content_eci_history_plugin.cpp
             applied_research_content_eci_operation.cpp
             research_content_eci_history_api.cpp
           )

target_link_libraries( deip_research_content_eci_history
                       deip_chain
                       deip_protocol
                       deip_app )
target_include_directories( deip_research_content_eci_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_rc_eci_history_manifest SOURCES plugin.json)

install( TARGETS
   deip_research_content_eci_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)