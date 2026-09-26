#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

// Snapshot compatible with cs2-dumper output format.
// Source requested by project owner: https://github.com/simv0lofficial/cs2_dumper
// Values are from the current public generated cs2-dumper data for CS2 build 14182
// (generated 2026-09-25 21:58:33 UTC). Keep this file small and hand-curated:
// only offsets used by this project are mirrored here.

namespace cs2_dumper::generated {

	inline constexpr std::string_view generated_at{ "2026-09-25 21:58:33 UTC" };
	inline constexpr int game_build{ 14182 };

	namespace offsets {

		namespace client_dll {
			constexpr std::ptrdiff_t dwCSGOInput = 0x2575BB0;
			constexpr std::ptrdiff_t dwEntityList = 0x27151A8;
			constexpr std::ptrdiff_t dwGameEntitySystem = 0x27151A8;
			constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x2120;
			constexpr std::ptrdiff_t dwGameRules = 0x255C8D8;
			constexpr std::ptrdiff_t dwGlobalVars = 0x222BF88;
			constexpr std::ptrdiff_t dwGlowManager = 0x255C8F0;
			constexpr std::ptrdiff_t dwLocalPlayerController = 0x2537628;
			constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x25606D8;
			constexpr std::ptrdiff_t dwPlantedC4 = 0x24C9290;
			constexpr std::ptrdiff_t dwPrediction = 0x25605E0;
			constexpr std::ptrdiff_t dwSensitivity = 0x255C820;
			constexpr std::ptrdiff_t dwSensitivity_sensitivity = 0x58;
			constexpr std::ptrdiff_t dwViewAngles = 0x2576238;
			constexpr std::ptrdiff_t dwViewMatrix = 0x2565A20;
			constexpr std::ptrdiff_t dwViewRender = 0x25662E0;
			constexpr std::ptrdiff_t dwWeaponC4 = 0x24C4550;
		} // namespace client_dll

		namespace engine2_dll {
			constexpr std::ptrdiff_t dwBuildNumber = 0x61D1E8;
			constexpr std::ptrdiff_t dwNetworkGameClient = 0x91B1C0;
			constexpr std::ptrdiff_t dwNetworkGameClient_clientTickCount = 0x398;
			constexpr std::ptrdiff_t dwNetworkGameClient_deltaTick = 0x24C;
			constexpr std::ptrdiff_t dwNetworkGameClient_isBackgroundMap = 0x2C143F;
			constexpr std::ptrdiff_t dwNetworkGameClient_localPlayer = 0xF8;
			constexpr std::ptrdiff_t dwNetworkGameClient_maxClients = 0x240;
			constexpr std::ptrdiff_t dwNetworkGameClient_serverTickCount = 0x24C;
			constexpr std::ptrdiff_t dwNetworkGameClient_signOnState = 0x230;
			constexpr std::ptrdiff_t dwWindowHeight = 0x91F544;
			constexpr std::ptrdiff_t dwWindowWidth = 0x91F540;
		} // namespace engine2_dll

		namespace inputsystem_dll {
			constexpr std::ptrdiff_t dwInputSystem = 0x46BC0;
		} // namespace inputsystem_dll

		namespace soundsystem_dll {
			constexpr std::ptrdiff_t dwSoundSystem = 0x535340;
			constexpr std::ptrdiff_t dwSoundSystem_engineViewData = 0x6C;
		} // namespace soundsystem_dll

	} // namespace offsets

	namespace buttons {
		constexpr std::ptrdiff_t attack = 0x22300C0;
		constexpr std::ptrdiff_t attack2 = 0x2230150;
		constexpr std::ptrdiff_t back = 0x2230390;
		constexpr std::ptrdiff_t duck = 0x2230660;
		constexpr std::ptrdiff_t forward = 0x2230300;
		constexpr std::ptrdiff_t jump = 0x22305D0;
		constexpr std::ptrdiff_t left = 0x2230420;
		constexpr std::ptrdiff_t lookatweapon = 0x2575AD0;
		constexpr std::ptrdiff_t reload = 0x2230030;
		constexpr std::ptrdiff_t right = 0x22304B0;
		constexpr std::ptrdiff_t showscores = 0x25759B0;
		constexpr std::ptrdiff_t sprint = 0x222FFA0;
		constexpr std::ptrdiff_t turnleft = 0x22301E0;
		constexpr std::ptrdiff_t turnright = 0x2230270;
		constexpr std::ptrdiff_t use = 0x2230540;
		constexpr std::ptrdiff_t zoom = 0x2575A40;
	} // namespace buttons

	namespace interfaces {

		[[nodiscard]] inline std::ptrdiff_t offset(
			std::string_view module_name,
			std::string_view interface_name ) noexcept
		{
			if ( module_name == "client.dll" )
			{
				if ( interface_name == "Source2Client002" ) return 0x2559E40;
				if ( interface_name == "Source2ClientPrediction001" ) return 0x25605E0;
				if ( interface_name == "LegacyGameUI001" ) return 0x223C1E0;
			}
			else if ( module_name == "engine2.dll" )
			{
				if ( interface_name == "Source2EngineToClient001" ) return 0x6201F0;
				if ( interface_name == "NetworkClientService_001" ) return 0x91B120;
				if ( interface_name == "GameEventSystemClientV001" ) return 0x91CBE0;
			}
			else if ( module_name == "panorama.dll" )
			{
				if ( interface_name == "PanoramaUIEngine001" ) return 0x587160;
			}
			else if ( module_name == "scenesystem.dll" )
			{
				if ( interface_name == "SceneSystem_002" ) return 0x91FCE0;
			}
			else if ( module_name == "materialsystem2.dll" )
			{
				if ( interface_name == "VMaterialSystem2_001" ) return 0x163730;
			}
			else if ( module_name == "schemasystem.dll" )
			{
				if ( interface_name == "SchemaSystem_001" ) return 0x76710;
			}
			else if ( module_name == "inputsystem.dll" )
			{
				if ( interface_name == "InputSystemVersion001" ) return 0x46BC0;
				if ( interface_name == "InputStackSystemVersion001" ) return 0x44E90;
			}
			else if ( module_name == "particles.dll" )
			{
				if ( interface_name == "ParticleSystemMgr003" ) return 0x65AEF0;
			}
			else if ( module_name == "tier0.dll" )
			{
				if ( interface_name == "VEngineCvar007" ) return 0x3AC670;
				if ( interface_name == "VStringTokenSystem001" ) return 0x3D3300;
			}
			else if ( module_name == "resourcesystem.dll" )
			{
				if ( interface_name == "ResourceSystem013" ) return 0x892A0;
			}
			else if ( module_name == "localize.dll" )
			{
				if ( interface_name == "Localize_001" ) return 0x59120;
			}
			else if ( module_name == "meshsystem.dll" )
			{
				if ( interface_name == "MeshSystem001" ) return 0x180AB0;
			}
			else if ( module_name == "filesystem_stdio.dll" )
			{
				if ( interface_name == "VFileSystem017" ) return 0x2143D0;
			}
			else if ( module_name == "soundsystem.dll" )
			{
				if ( interface_name == "SoundSystem001" ) return 0x535340;
			}
			else if ( module_name == "vphysics2.dll" )
			{
				if ( interface_name == "VPhysics2_Interface_001" ) return 0x460E70;
			}

			return 0;
		}

		[[nodiscard]] inline std::uintptr_t address(
			std::uintptr_t module_base,
			std::string_view module_name,
			std::string_view interface_name ) noexcept
		{
			const auto rva = offset( module_name, interface_name );
			return module_base && rva > 0
				? module_base + static_cast< std::uintptr_t >( rva )
				: 0;
		}

	} // namespace interfaces

} // namespace cs2_dumper::generated
