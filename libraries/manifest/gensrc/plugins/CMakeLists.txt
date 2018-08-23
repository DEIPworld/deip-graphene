
# The deip_plugins target links with all plugins
add_library( deip_plugins
   plugins.cpp )

target_link_libraries( deip_plugins

   deip_account_by_key

   deip_blockchain_history

   deip_account_statistics

   deip_auth_util

   deip_block_info

   deip_blockchain_statistics

   deip_debug_node

   deip_delayed_node

   deip_raw_block

   deip_witness

)

# The `deip_mf_plugins` target has prototypes for querying plugins
# as abstract, but does not itself define the plugins.
#
# The reason for this design is that if we try to use `plugins`
# we will have this circular dependency:
#
# ```
# xxx_plugin ===> deip::app ===> plugins ===> xxx_plugin
# ```
#
# To break the dependency, we create `mf_plugins` which only
# has a "weak" reference to the plugin:
#
# ```
# xxx_plugin ===> deip::app ===> mf_plugins ---> xxx_plugin
# ```
#
# This weak reference is implemented by a declaring a
# `create_xxx_plugin()` global function which returns a
# `shared_ptr< abstract_plugin >`.  The weak reference isn't
# resolved until link time.  So as long as we link with both
# `deip_mf_plugins` and `deip_plugins` we are fine.
#

add_library( deip_mf_plugins
   mf_plugins.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/include/deip/manifest/plugins.hpp
)

target_include_directories( deip_mf_plugins
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
   )
target_link_libraries( deip_mf_plugins fc )

INSTALL( TARGETS
   deip_mf_plugins

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)