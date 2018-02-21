file(GLOB HEADERS "include/deip/app/*.hpp")
file(GLOB EGENESIS_HEADERS "../egenesis/include/deip/app/*.hpp")

add_library( deip_app
             database_api.cpp
             api.cpp
             application.cpp
             impacted.cpp
             plugin.cpp
             ${HEADERS}
             ${EGENESIS_HEADERS}
           )

target_link_libraries( deip_app deip_chain deip_protocol deip_tags deip_witness deip_mf_plugins fc graphene_net graphene_utilities )
target_include_directories( deip_app
                            PUBLIC
                            "${CMAKE_CURRENT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/../egenesis/include")

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   deip_app

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/deip/app" )
