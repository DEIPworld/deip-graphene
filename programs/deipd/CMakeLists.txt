add_executable(deipd main.cpp)
if(UNIX AND NOT APPLE)
    set(rt_library rt)
endif()

find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling deipd with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

target_link_libraries(deipd PRIVATE
    deip_plugins
    deip_mf_plugins
    deip_app
    deip_witness
    deip_blockchain_history
    deip_chain
    deip_protocol
    deip_egenesis_full
    fc
    deip_tsc_history
    deip_research_content_reference_history
    deip_eci_history
    deip_fo_history
    ${CMAKE_DL_LIBS}
    ${PLATFORM_SPECIFIC_LIBS}
)

install(TARGETS
    deipd

    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
