file(GLOB HEADERS "include/deip/account_statistics/*.hpp")

add_library( deip_account_statistics
             account_statistics_plugin.cpp
             account_statistics_api.cpp
           )

target_link_libraries( deip_account_statistics deip_chain deip_protocol deip_app )
target_include_directories( deip_account_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_custom_target( deip_account_statistics_manifest SOURCES plugin.json)

install( TARGETS
   deip_account_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)