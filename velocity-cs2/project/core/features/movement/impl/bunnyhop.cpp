#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <core/systems/systems.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>

#include "../movement.hpp"
#include <protection/game_addresses.hpp>

namespace features::movement {

	namespace {

		constexpr auto k_jump_button = cstypes::command_buttons::in_jump;
		constexpr auto k_subtick_step = 1.0f / 64.0f;

		[[nodiscard]] float clamp_subtick_time( float when )
		{
			return std::clamp( when, k_subtick_step, 1.0f - k_subtick_step );
		}

		void write_jump_state(
			systems::input::usercmd* cmd,
			bool pressed,
			bool mark_changed )
		{
			if ( !cmd )
			{
				return;
			}

			if ( pressed )
			{
				cmd->buttons.value |= k_jump_button;
				cmd->buttons.value_scroll |= k_jump_button;
			}
			else
			{
				cmd->buttons.value &= ~k_jump_button;
				cmd->buttons.value_scroll &= ~k_jump_button;
			}

			if ( mark_changed )
			{
				cmd->buttons.value_changed |= k_jump_button;
			}
		}

		[[nodiscard]] bool has_jump_subtick( const proto::base_usercmd_pb* base )
		{
			if ( !base )
			{
				return false;
			}

			const auto& moves = base->subtick_moves( );
			if ( !moves.m_rep )
			{
				return false;
			}

			for ( auto i = 0; i < moves.m_current_size; ++i )
			{
				const auto step = proto::impl_ptr<const proto::subtick_move_step>(
					moves.m_rep->elements[ i ] );
				if ( step && step->m_has_bits.test( 0x1u ) && step->button( ) == k_jump_button )
				{
					return true;
				}
			}

			return false;
		}

		[[nodiscard]] bool add_jump_subtick(
			proto::repeated_ptr_field<proto::subtick_move_step>* subtick_moves,
			bool pressed,
			float when )
		{
			const auto step = systems::g_input.acquire_subtick_step( subtick_moves );
			if ( !step )
			{
				return false;
			}

			step->set_button( k_jump_button );
			step->set_pressed( pressed );
			step->set_when( clamp_subtick_time( when ) );
			step->set_analog_forward_delta( 0.0f );
			step->set_analog_left_delta( 0.0f );
			return true;
		}

		[[nodiscard]] std::optional<float> predict_landing_fraction(
			std::uintptr_t local_pawn,
			std::uintptr_t movement_services,
			const systems::prediction::state& prestate,
			bool holding_duck )
		{
			if ( !local_pawn || !movement_services || prestate.networked_velocity.z > 0.0f )
			{
				return std::nullopt;
			}

			const auto duck_amount_offset = SCHEMA( "CCSPlayer_MovementServices", "m_flDuckAmount"_hash );
			const auto collision_offset = SCHEMA( "C_BaseModelEntity", "m_Collision"_hash );
			const auto mins_offset = SCHEMA( "CCollisionProperty", "m_vecMins"_hash );
			const auto maxs_offset = SCHEMA( "CCollisionProperty", "m_vecMaxs"_hash );
			const auto gravity_scale_offset = SCHEMA( "C_BaseEntity", "m_flGravityScale"_hash );
			if ( duck_amount_offset <= 0 || collision_offset <= 0 || mins_offset <= 0 ||
				maxs_offset <= 0 || gravity_scale_offset <= 0 )
			{
				return std::nullopt;
			}

			const auto duck_amount = memory::safe_read<float>(
				movement_services + duck_amount_offset ).value_or( 0.0f );
			const auto collision = local_pawn + collision_offset;
			const auto mins = memory::safe_read<math::vector3>( collision + mins_offset );
			auto maxs = memory::safe_read<math::vector3>( collision + maxs_offset );
			if ( !mins || !maxs )
			{
				return std::nullopt;
			}

			auto trace_origin = prestate.networked_origin;
			if ( holding_duck && duck_amount > 0.0f )
			{
				const auto standing_height{ 72.0f };
				const auto duck_hull_diff = standing_height - maxs->z;
				trace_origin.z -= duck_hull_diff * 0.5f;
				maxs->z = standing_height;
			}

			auto trace_mask{ 0ull };
			const auto pawn_ptr = memory::safe_read<std::uintptr_t>( movement_services + 56 ).value_or( 0 );
			if ( pawn_ptr )
			{
				trace_mask = memory::safe_read<std::uintptr_t>( pawn_ptr + 0xd48 ).value_or( 0 );
				const auto pawn_flags = memory::safe_read<std::uint32_t>( pawn_ptr + 0x3f8 ).value_or( 0 );
				if ( pawn_flags & 0x10 )
				{
					trace_mask |= 0x20;
				}
			}
			else
			{
				trace_mask |= 0x20;
			}

			const auto sv_gravity_cvar = CONVAR( "sv_gravity" );
			const auto sv_standable_normal_cvar = CONVAR( "sv_standable_normal" );
			const auto sv_gravity = sv_gravity_cvar ? sv_gravity_cvar->get<float>( ) : 800.0f;
			const auto sv_standable_normal = sv_standable_normal_cvar ? sv_standable_normal_cvar->get<float>( ) : 0.7f;
			const auto gravity_scale = memory::safe_read<float>(
				local_pawn + gravity_scale_offset ).value_or( 1.0f );

			const auto filter = systems::g_tracing.make_player_movement_filter( local_pawn, trace_mask, 11 );
			auto velocity = prestate.networked_velocity;
			velocity.z -= ( gravity_scale * sv_gravity * cstypes::tick_interval ) * 0.5f;

			const math::vector3 trace_start = trace_origin;
			math::vector3 trace_end{};

			trace_end.x = trace_origin.x + velocity.x * cstypes::tick_interval;
			trace_end.y = trace_origin.y + velocity.y * cstypes::tick_interval;
			trace_end.z = trace_origin.z + velocity.z * cstypes::tick_interval;
			trace_end.z -= 2.0f;

			const auto result = systems::g_tracing.trace_player_bbox(
				trace_start,
				trace_end,
				{ *mins, *maxs },
				filter,
				movement_services );
			if ( result.fraction <= 0.0f || result.fraction >= 1.0f ||
				result.normal.z < sv_standable_normal )
			{
				return std::nullopt;
			}

			return clamp_subtick_time( std::round( result.fraction * 64.0f ) / 64.0f );
		}

		[[nodiscard]] bool apply_landing_jump( proto::base_usercmd_pb* base, float when )
		{
			if ( !base || has_jump_subtick( base ) )
			{
				return false;
			}

			const auto subtick_moves = base->mutable_subtick_moves( );
			const auto jump_when = clamp_subtick_time( when );
			const auto release_when = clamp_subtick_time( jump_when - k_subtick_step );

			if ( release_when < jump_when )
			{
				static_cast<void>( add_jump_subtick( subtick_moves, false, release_when ) );
			}

			return add_jump_subtick( subtick_moves, true, jump_when );
		}

	} // namespace

	void bhop::on_create_move( systems::input::usercmd* cmd )
	{
		if ( !cmd || !settings::g_movement.bhop.value )
		{
			this->m_suppressed_air_jump = false;
			return;
		}

		const auto sv_autobunnyhopping = CONVAR( "sv_autobunnyhopping" );
		if ( sv_autobunnyhopping && sv_autobunnyhopping->get<bool>( ) )
		{
			this->m_suppressed_air_jump = false;
			return;
		}

		if ( !( cmd->buttons.value & k_jump_button ) )
		{
			this->m_suppressed_air_jump = false;
			return;
		}

		if ( features::movement::g_jumpbug.active_this_tick( ) )
		{
			return;
		}

		const auto local = systems::g_local.get( );
		if ( !local.pawn )
		{
			this->m_suppressed_air_jump = false;
			return;
		}

		const auto move_type_offset = SCHEMA( "C_BaseEntity", "m_nActualMoveType"_hash );
		if ( move_type_offset <= 0 )
		{
			this->m_suppressed_air_jump = false;
			return;
		}

		const auto move_type = memory::safe_read<std::uint8_t>(
			local.pawn + move_type_offset ).value_or( cstypes::move_type::none );
		if ( move_type == cstypes::move_type::ladder ||
			move_type == cstypes::move_type::noclip ||
			move_type == cstypes::move_type::observer )
		{
			this->m_suppressed_air_jump = false;
			return;
		}

		const auto water_level_offset = SCHEMA( "C_BaseEntity", "m_flWaterLevel"_hash );
		if ( water_level_offset > 0 &&
			memory::safe_read<float>( local.pawn + water_level_offset ).value_or( 0.0f ) > 1.0f )
		{
			this->m_suppressed_air_jump = false;
			return;
		}

		const auto& prestate = systems::g_prediction.pre( );
		if ( prestate.flags & cstypes::entity_flags::on_ground )
		{
			if ( this->m_suppressed_air_jump )
			{
				write_jump_state( cmd, true, true );
			}

			this->m_suppressed_air_jump = false;
			return;
		}

		write_jump_state( cmd, false, !this->m_suppressed_air_jump );
		this->m_suppressed_air_jump = true;

		const auto movement_services_offset = SCHEMA( "C_BasePlayerPawn", "m_pMovementServices"_hash );
		if ( movement_services_offset <= 0 )
		{
			return;
		}

		const auto movement_services = memory::safe_read<std::uintptr_t>(
			local.pawn + movement_services_offset ).value_or( 0 );
		if ( !movement_services )
		{
			return;
		}

		const auto holding_duck = ( cmd->buttons.value & cstypes::command_buttons::in_duck ) != 0;
		const auto landing = predict_landing_fraction( local.pawn, movement_services, prestate, holding_duck );
		if ( !landing )
		{
			return;
		}

		const auto base = cmd->csgo_user_cmd.mutable_base( );
		if ( !base )
		{
			return;
		}

		if ( apply_landing_jump( base, *landing ) )
		{
			write_jump_state( cmd, true, true );
		}
	}

} // namespace features::movement
