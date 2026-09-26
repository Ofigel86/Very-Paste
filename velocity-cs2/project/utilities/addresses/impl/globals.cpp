#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/logging/logging.hpp>
#include <protection/game_addresses.hpp>
#include <protection/cs2_dumper_offsets.hpp>
#include "../addresses.hpp"

namespace addresses::globals {

	namespace {

		[[nodiscard]] std::uintptr_t rva_address( std::uintptr_t module_base, std::ptrdiff_t rva )
		{
			return module_base && rva > 0
				? module_base + static_cast<std::uintptr_t>( rva )
				: 0;
		}

		[[nodiscard]] std::uintptr_t pattern_or_dumper(
			const ::protection::addresses::address_t& pattern,
			std::uintptr_t module_base,
			std::ptrdiff_t rva,
			std::string_view name )
		{
			// PATTERN(...) wraps its argument in a captureless lambda and therefore
			// cannot be used with a runtime function parameter on MSVC. Resolve the
			// pattern string directly here; this helper is called once during address
			// initialization, so we do not need the macro's per-call-site static cache.
			if ( pattern.data.data )
			{
				if ( const auto resolved = memory::resolve_pattern( pattern.data.data ) )
					return resolved;
			}

			const auto fallback = rva_address( module_base, rva );
			if ( fallback )
			{
				logging::console::print(
					xs( "[addresses] pattern fallback from cs2-dumper | {} -> {:#x}" ),
					name,
					fallback );
			}

			return fallback;
		}

	} // namespace

	bool initialize () {
		source2client              = INTERFACE_ ("client.dll:Source2Client002");
		panorama                   = INTERFACE_ ("panorama.dll:PanoramaUIEngine001");
		source2engine_to_client    = INTERFACE_ ("engine2.dll:Source2EngineToClient001");
		scene_system               = INTERFACE_ ("scenesystem.dll:SceneSystem_002");
		material_system            = INTERFACE_ ("materialsystem2.dll:VMaterialSystem2_001");
		schema_system              = INTERFACE_ ("schemasystem.dll:SchemaSystem_001");
		input_system               = INTERFACE_ ("inputsystem.dll:InputSystemVersion001");
		particle_system_mgr        = INTERFACE_ ("particles.dll:ParticleSystemMgr003");
		cvar                       = reinterpret_cast<interfaces::c_engine_cvar*>( INTERFACE_ ("tier0.dll:VEngineCvar007") );
		source2client_prediction   = INTERFACE_ ("client.dll:Source2ClientPrediction001");
		network_client_service     = INTERFACE_ ("engine2.dll:NetworkClientService_001");
		resource_system            = INTERFACE_ ("resourcesystem.dll:ResourceSystem013");
		localize                   = INTERFACE_ ("localize.dll:Localize_001");
		mesh_system                = INTERFACE_ ("meshsystem.dll:MeshSystem001");
		file_system                = INTERFACE_ ("filesystem_stdio.dll:VFileSystem017");

		csgo_input             = pattern_or_dumper( patterns::csgo_input, addresses::modules::client, cs2_dumper::generated::offsets::client_dll::dwCSGOInput, "csgo_input" );
		entity_list            = pattern_or_dumper( patterns::entity_list, addresses::modules::client, cs2_dumper::generated::offsets::client_dll::dwEntityList, "entity_list" );
		local_player_controller = pattern_or_dumper( patterns::local_player_controller, addresses::modules::client, cs2_dumper::generated::offsets::client_dll::dwLocalPlayerController, "local_player_controller" );
		global_vars            = pattern_or_dumper( patterns::global_vars, addresses::modules::client, cs2_dumper::generated::offsets::client_dll::dwGlobalVars, "global_vars" );
		view_matrix            = pattern_or_dumper( patterns::view_matrix, addresses::modules::client, cs2_dumper::generated::offsets::client_dll::dwViewMatrix, "view_matrix" );
		game_rules             = pattern_or_dumper( patterns::game_rules, addresses::modules::client, cs2_dumper::generated::offsets::client_dll::dwGameRules, "game_rules" );
		light_data_queue       = PATTERN (patterns::light_data_queue);
		particle_manager       = PATTERN (patterns::particle_manager);
		game_event_manager     = PATTERN (patterns::game_event_manager);
		game_trace_manager     = PATTERN (patterns::game_trace_manager);
		render_game_system_storage = PATTERN (patterns::render_game_system_storage);
		material_manager       = PATTERN (patterns::material_manager);
		game_entity_system     = pattern_or_dumper( patterns::game_entity_system, addresses::modules::client, cs2_dumper::generated::offsets::client_dll::dwGameEntitySystem, "game_entity_system" );
		weapon_recoil_data     = PATTERN (patterns::weapon_recoil_data);
		hud                    = PATTERN (patterns::hud);
		prediction_seed        = PATTERN (patterns::prediction_seed);
		simulation_player      = PATTERN (patterns::simulation_player);
		prediction_player      = PATTERN (patterns::prediction_player);
		planted_c4             = pattern_or_dumper( patterns::planted_c4, addresses::modules::client, cs2_dumper::generated::offsets::client_dll::dwPlantedC4, "planted_c4" );
		item_system            = PATTERN (patterns::item_system);
		frame_input_ring_idx   = PATTERN (patterns::frame_input_ring_idx);
		frame_input_ring_base  = PATTERN (patterns::frame_input_ring_base);
		prediction_state       = PATTERN (patterns::prediction_state);

		if (const auto ptr = MODULE_EXPORT("tier0.dll:g_pMemAlloc"))
			mem_alloc = *reinterpret_cast<std::uintptr_t*>(ptr);

		const std::pair<std::string_view, std::uintptr_t> required_interfaces[] {
			{ "source2client", source2client },
			{ "panorama", panorama },
			{ "source2engine_to_client", source2engine_to_client },
			{ "scene_system", scene_system },
			{ "material_system", material_system },
			{ "schema_system", schema_system },
			{ "input_system", input_system },
			{ "particle_system_mgr", particle_system_mgr },
			{ "cvar", reinterpret_cast<std::uintptr_t>( cvar ) },
			{ "source2client_prediction", source2client_prediction },
			{ "network_client_service", network_client_service },
			{ "resource_system", resource_system },
			{ "localize", localize },
			{ "mesh_system", mesh_system },
			{ "file_system", file_system },
		};

		const std::pair<std::string_view, std::uintptr_t> required_globals[] {
			{ "csgo_input", csgo_input },
			{ "entity_list", entity_list },
			{ "local_player_controller", local_player_controller },
			{ "global_vars", global_vars },
			{ "view_matrix", view_matrix },
			{ "game_rules", game_rules },
			{ "light_data_queue", light_data_queue },
			{ "particle_manager", particle_manager },
			{ "game_event_manager", game_event_manager },
			{ "game_trace_manager", game_trace_manager },
			{ "render_game_system_storage", render_game_system_storage },
			{ "mem_alloc", mem_alloc },
			{ "material_manager", material_manager },
			{ "game_entity_system", game_entity_system },
			{ "weapon_recoil_data", weapon_recoil_data },
			{ "hud", hud },
			{ "prediction_seed", prediction_seed },
			{ "simulation_player", simulation_player },
			{ "prediction_player", prediction_player },
			{ "planted_c4", planted_c4 },
			{ "item_system", item_system },
			{ "frame_input_ring_idx", frame_input_ring_idx },
			{ "frame_input_ring_base", frame_input_ring_base },
			{ "prediction_state", prediction_state },
		};

		auto initialized = true;
		for (const auto& [name, address] : required_interfaces) {
			if (!address) {
				logging::console::print (xs ("[error] interface not initialized | {}"), name);
				initialized = false;
			}
		}

		for (const auto& [name, address] : required_globals) {
			if (!address) {
				logging::console::print (xs ("[error] global address not initialized | {}"), name);
				initialized = false;
			}
		}

		return initialized;
	}

} // namespace addresses::globals
