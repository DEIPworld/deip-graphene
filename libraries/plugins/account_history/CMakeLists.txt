file(GLOB HEADERS "include/deip/account_history/*.hpp")

add_library( deip_account_history
             account_history_plugin.cpp
             account_history_api.cpp
           )

target_link_libraries( deip_account_history deip_chain deip_protocol deip_app )
target_include_directories( deip_account_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   deip_account_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
