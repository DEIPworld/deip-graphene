#include <boost/container/flat_map.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

#include <deip/manifest/plugins.hpp>

namespace deip { namespace plugin {


std::shared_ptr< deip::app::abstract_plugin > create_account_by_key_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_account_history_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_account_statistics_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_auth_util_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_block_info_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_blockchain_statistics_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_debug_node_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_delayed_node_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_raw_block_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_witness_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_tsc_history_plugin( deip::app::application* app );

std::shared_ptr< deip::app::abstract_plugin > create_cr_history_plugin( deip::app::application* app );

std::shared_ptr<deip::app::abstract_plugin> create_eci_history_plugin(deip::app::application* app);

std::shared_ptr< deip::app::abstract_plugin > create_funding_opportunity_history_plugin( deip::app::application* app );

boost::container::flat_map< std::string, std::function< std::shared_ptr< deip::app::abstract_plugin >( deip::app::application* app ) > > plugin_factories_by_name;

void initialize_plugin_factories()
{
   
   plugin_factories_by_name[ "account_by_key" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_account_by_key_plugin( app );
   };
   
   plugin_factories_by_name[ "account_history" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_account_history_plugin( app );
   };
   
   plugin_factories_by_name[ "account_statistics" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_account_statistics_plugin( app );
   };
   
   plugin_factories_by_name[ "auth_util" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_auth_util_plugin( app );
   };
   
   plugin_factories_by_name[ "block_info" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_block_info_plugin( app );
   };
   
   plugin_factories_by_name[ "blockchain_statistics" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_blockchain_statistics_plugin( app );
   };
   
   plugin_factories_by_name[ "debug_node" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_debug_node_plugin( app );
   };
   
   plugin_factories_by_name[ "delayed_node" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_delayed_node_plugin( app );
   };
   
   plugin_factories_by_name[ "raw_block" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_raw_block_plugin( app );
   };
   
   plugin_factories_by_name[ "witness" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_witness_plugin( app );
   };

   plugin_factories_by_name[ "tsc_history" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_tsc_history_plugin( app );
   };

   plugin_factories_by_name[ "research_content_references_history" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_cr_history_plugin( app );
   };

   plugin_factories_by_name[ "eci_history" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_eci_history_plugin( app );
   };

   plugin_factories_by_name[ "fo_history" ] = []( deip::app::application* app ) -> std::shared_ptr< deip::app::abstract_plugin >
   {
      return create_funding_opportunity_history_plugin( app );
   };

   
}

std::shared_ptr< deip::app::abstract_plugin > create_plugin( const std::string& name, deip::app::application* app )
{
   auto it = plugin_factories_by_name.find( name );
   if( it == plugin_factories_by_name.end() )
      return std::shared_ptr< deip::app::abstract_plugin >();
   return it->second( app );
}

std::vector< std::string > get_available_plugins()
{
   std::vector< std::string > result;
   for( const auto& e : plugin_factories_by_name )
      result.push_back( e.first );
   return result;
}

} }