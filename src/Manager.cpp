#include "Manager.h"

void AutoSandboxHandler::Register()
{
	if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
		inputMgr->AddEventSink<RE::InputEvent*>(this);
		registeredForInput = true;
		logger::info("Registered for hotkey event");
	}

	if (auto scripts = RE::ScriptEventSourceHolder::GetSingleton()) {
		scripts->AddEventSink<RE::TESLoadGameEvent>(this);
		logger::info("Registered for load game event");
	}

	exteriorPackage = RE::TESForm::LookupByEditorID<RE::TESPackage>("SandboxWhenIdleExterior");
	interiorPackage = RE::TESForm::LookupByEditorID<RE::TESPackage>("SandboxWhenIdleInterior");

	furnitureForceFirstPerson = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSKeyword>(RE::DEFAULT_OBJECT::kKeywordFurnitureForces1stPerson);
	furnitureForceThirdPerson = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSKeyword>(RE::DEFAULT_OBJECT::kKeywordFurnitureForces3rdPerson);
	furnitureSpecial = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSKeyword>(RE::DEFAULT_OBJECT::kKeywordSpecialFurniture);
	resetRootIdle = RE::TESForm::LookupByEditorID<RE::TESIdleForm>("ResetRoot");

	improvedCameraInstalled = GetModuleHandle(L"ImprovedCameraSE.dll") != nullptr;
}

bool AutoSandboxHandler::IsAutoSandboxing() const
{
	return autoSandBoxing;
}

bool AutoSandboxHandler::CanSandbox() const
{
	if (!registeredForInput || sandboxCheckFailed) {
		return false;
	}

	const auto player = RE::PlayerCharacter::GetSingleton();
	if (!player->currentProcess || player->IsDead() || player->IsInCombat() || player->IsSneaking() || player->IsOnMount() || !player->GetPlayerControls()) {
		return false;
	}

	const auto controlMap = RE::ControlMap::GetSingleton();
	if (controlMap && (!controlMap->IsMovementControlsEnabled() || !controlMap->IsFightingControlsEnabled() || !controlMap->IsPOVSwitchControlsEnabled() || !controlMap->IsLookingControlsEnabled() || !controlMap->IsSneakingControlsEnabled() || !controlMap->IsMenuControlsEnabled())) {
		return false;
	} 

	const auto processLists = RE::ProcessLists::GetSingleton();
	if (processLists && processLists->AreHostileActorsNear(nullptr)) {
		return false;
	}

	return true;
}

bool AutoSandboxHandler::StartSandbox()
{
	if (!autoSandBoxing) {
		if (CanSandbox()) {
			if (auto package = GetPackage()) {
				autoSandBoxing = true;

				auto player = RE::PlayerCharacter::GetSingleton();
				player->DrawWeaponMagicHands(false);
				player->SetAIDriven(true);
				player->currentProcess->SetRunOncePackage(nullptr, player);
				player->currentProcess->ComputeLastTimeProcessed();
				player->PutCreatedPackage(package, false, false);
				player->EvaluatePackage();

				SetHeadTracking(true);
				UpdateCameraForSandbox(true);
			}
		} else {
			sandboxCheckFailed = true;
		}
	}

	return autoSandBoxing;
}

void AutoSandboxHandler::StopSandbox()
{
	autoSandBoxing = false;

	auto player = RE::PlayerCharacter::GetSingleton();
	player->StopInteractingQuick(true);
	if (auto currentProcess = player->currentProcess) {
		currentProcess->StopCurrentIdle(player, true);
		currentProcess->PlayIdle(player, resetRootIdle, nullptr);

		currentProcess->AddToProcedureIndexRunning(player, 1);
		currentProcess->SetRunOncePackage(nullptr, player);
	}
	player->SetAIDriven(false);

	SetHeadTracking(false);
	UpdateCameraForSandbox(false);
}

void AutoSandboxHandler::ResetSandboxCheck()
{
	sandboxCheckFailed = false;
}

void AutoSandboxHandler::UpdateCameraForSandbox(bool a_enable)
{
	auto playerCamera = RE::PlayerCamera::GetSingleton();
	if (a_enable) {
		isInFirstPerson = !improvedCameraInstalled && playerCamera->IsInFirstPerson();  // don't switch if IC is installed
		if (isInFirstPerson) {
			playerCamera->PushCameraState(RE::CameraState::kThirdPerson);
			if (auto tps = skyrim_cast<RE::ThirdPersonState*>(RE::PlayerCamera::GetSingleton()->currentState.get())) {
				// default tps is too zoomed in
				currentZoom = tps->currentZoomOffset;
				targetZoom = tps->targetZoomOffset;
				if (currentZoom < 0.8f) {
					tps->currentZoomOffset = 0.8f;
				}
				if (targetZoom < 0.8f) {
					tps->targetZoomOffset = 0.8f;
				}
			}
		}
		RE::SendHUDMessage::PushHUDMode("AutoVanity");
	} else {
		if (isInFirstPerson) {
			playerCamera->ForceFirstPerson();
		}
		playerCamera->idleTimer = 0.0f;
		RE::SendHUDMessage::PopHUDMode("AutoVanity");
	}
}

void AutoSandboxHandler::SetHeadTracking(bool a_enable)
{
	auto player = RE::PlayerCharacter::GetSingleton();

	if (a_enable) {
		headTrackingState = player->actorState2.headTracking;
		player->GetGraphVariableBool("IsNPC", isNPCMode);

		player->actorState2.headTracking = true;
		player->SetGraphVariableBool("IsNPC", true);
	} else {
		player->actorState2.headTracking = headTrackingState;
		player->SetGraphVariableBool("IsNPC", isNPCMode);
	}
}

RE::TESPackage* AutoSandboxHandler::GetPackage()
{
	auto parentCell = RE::PlayerCharacter::GetSingleton()->parentCell;
	return parentCell && parentCell->IsInteriorCell() ? interiorPackage : exteriorPackage;
}

RE::BSEventNotifyControl AutoSandboxHandler::ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*)
{
	if (!a_evn || !IsAutoSandboxing()) {
		return RE::BSEventNotifyControl::kContinue;
	}

	for (auto event = *a_evn; event; event = event->next) {
		if (const auto buttonEvent = event->AsButtonEvent(); buttonEvent && buttonEvent->IsDown()) {
			StopSandbox();
			break;
		}
	}

	return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl AutoSandboxHandler::ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*)
{
	if (!a_evn) {
		return RE::BSEventNotifyControl::kContinue;
	}

	if (IsAutoSandboxing()) {
		StopSandbox();
	}

	return RE::BSEventNotifyControl::kContinue;
}
