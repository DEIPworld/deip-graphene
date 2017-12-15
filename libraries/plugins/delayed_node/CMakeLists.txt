file(GLOB HEADERS "include/deip/delayed_node/*.hpp")

add_library( deip_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( deip_delayed_node deip_chain deip_protocol deip_app )
target_include_directories( deip_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   deip_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
