file(GLOB HEADERS "include/deip/follow/*.hpp")

add_library( deip_follow
             follow_plugin.cpp
             follow_api.cpp
             follow_operations.cpp
             follow_evaluators.cpp
           )

target_link_libraries( deip_follow deip_chain deip_protocol deip_app )
target_include_directories( deip_follow
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   deip_follow

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
