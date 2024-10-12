#include "Hooks.h"

#include "Manager.h"

namespace Hooks
{
	struct AutoStartVanityMode
	{
		static void thunk([[maybe_unused]] RE::PlayerCamera* a_camera)
		{
			if (!AutoSandboxHandler::GetSingleton()->StartSandbox()) {
				func(a_camera);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(49852, 50784), OFFSET(0x15E, 0x154) };  // PlayerCamera::Update
			stl::write_thunk_call<AutoStartVanityMode>(target.address());
		}
	};

	struct AutoVanityStateEnd
	{
		static void thunk(RE::AutoVanityState* a_this)
		{
			func(a_this);
			AutoSandboxHandler::GetSingleton()->ResetSandboxCheck();
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            idx = 0x2;
	};

	struct CanInteractWith
	{
		static bool thunk(RE::TESFurniture* a_this, RE::Actor* a_actor)
		{
			if (a_actor && a_actor->IsPlayerRef() && AutoSandboxHandler::GetSingleton()->IsAutoSandboxing()) {
				return a_this && a_this->workBenchData.benchType.get() == RE::TESFurniture::WorkBenchData::BenchType::kNone;
			}
			return func(a_this, a_actor);
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(28275, 29021) };  // BGSProcedureSandboxExecState::EvaluateReferenceforUse
			stl::write_thunk_call<CanInteractWith>(target.address() + OFFSET(0x2AD, 0x221));
		}
	};

	void Install()
	{
		AutoStartVanityMode::Install();

		stl::write_vfunc<RE::AutoVanityState, AutoVanityStateEnd>();

		CanInteractWith::Install();
	}
}
