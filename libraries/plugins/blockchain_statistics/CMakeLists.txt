file(GLOB HEADERS "include/deip/blockchain_statistics/*.hpp")

add_library( deip_blockchain_statistics
             blockchain_statistics_plugin.cpp
             blockchain_statistics_api.cpp
           )

target_link_libraries( deip_blockchain_statistics deip_chain deip_protocol deip_app )
target_include_directories( deip_blockchain_statistics
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   deip_blockchain_statistics

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)