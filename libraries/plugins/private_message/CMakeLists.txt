file(GLOB HEADERS "include/deip/private_message/*.hpp")

add_library( deip_private_message
             private_message_plugin.cpp
           )

target_link_libraries( deip_private_message deip_chain deip_protocol deip_app )
target_include_directories( deip_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   deip_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
