file(GLOB HEADERS "include/deip/witness/*.hpp")

add_library( deip_witness
             witness_plugin.cpp
           )

target_link_libraries( deip_witness deip_chain deip_protocol deip_app )
target_include_directories( deip_witness
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_witness_manifest SOURCES plugin.json)

install( TARGETS
   deip_witness

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
