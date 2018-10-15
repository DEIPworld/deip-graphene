find_package(Gperftools QUIET)
if(GPERFTOOLS_FOUND)
    message(STATUS "Found gperftools; compiling tests with TCMalloc")
    list(APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
endif()

file(GLOB_RECURSE COMMON_SOURCES "common/*.cpp")

file(GLOB_RECURSE SOURCES "tests/*.cpp")

add_executable(chain_test ${SOURCES} ${COMMON_SOURCES} tests/offer_research_tokens_service_tests.cpp)
target_link_libraries(chain_test chainbase deip_chain deip_protocol deip_app deip_blockchain_history deip_witness deip_egenesis_none deip_debug_node fc ${PLATFORM_SPECIFIC_LIBS})
target_include_directories(chain_test PUBLIC "common")

file(GLOB_RECURSE WALLET_SOURCES "wallet/*.cpp")
add_executable(wallet_tests ${WALLET_SOURCES})
target_link_libraries(wallet_tests deip_wallet deip_chain deip_app deip_blockchain_history deip_egenesis_none graphene_utilities fc)

if(MSVC)
    set_source_files_properties(tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)