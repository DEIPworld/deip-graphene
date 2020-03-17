file(GLOB HEADERS "include/deip/protocol/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( deip_protocol

             types.cpp
             authority.cpp
             operations.cpp
             sign_state.cpp
             operation_util_impl.cpp
             deip_operations.cpp
             operations/create_research_group_operation.cpp
             operations/make_review_operation.cpp
             operations/create_grant_operation.cpp
             transaction.cpp
             block.cpp
             asset.cpp
             version.cpp
             get_config.cpp

             ${HEADERS}
           )

target_link_libraries( deip_protocol fc )
target_include_directories( deip_protocol
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   deip_protocol

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

INSTALL( FILES ${HEADERS} DESTINATION "include/deip/protocol" )
