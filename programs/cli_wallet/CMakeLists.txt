add_executable( cli_wallet main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

# I don't know why deip_app is required twice in the following line, I just know the linker breaks if it isn't.
target_link_libraries( cli_wallet
                       PRIVATE deip_app graphene_net deip_chain deip_egenesis_brief deip_protocol graphene_utilities deip_wallet deip_app deip_blockchain_history fc deip_tsc_history deip_research_content_reference_history deip_eci_history deip_fo_history ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   cli_wallet

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
