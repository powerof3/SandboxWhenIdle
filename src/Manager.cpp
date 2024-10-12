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

	auto player = RE::PlayerCharacter::GetSingleton();
	if (!player->currentProcess) {
		return false;
	}

	if (player->IsDead() || player->IsInCombat() || player->IsSneaking() || player->IsOnMount()) {
		return false;
	}

	if (auto processLists = RE::ProcessLists::GetSingleton(); processLists && processLists->AreHostileActorsNear(nullptr)) {
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
				player->PutCreatedPackage(package, true, false);

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
		}
		if (auto tps = skyrim_cast<RE::ThirdPersonState*>(RE::PlayerCamera::GetSingleton()->currentState.get())) {
			// default tps is too zoomed in
			currentZoom = tps->currentZoomOffset;
			targetZoom = tps->targetZoomOffset;
			
			tps->currentZoomOffset = 0.8f;
			tps->targetZoomOffset = 0.8f;
		}
		RE::SendHUDMessage::PushHUDMode("AutoVanity");
	} else {
		if (isInFirstPerson) {
			playerCamera->ForceFirstPerson();
		} else {
			if (auto tps = skyrim_cast<RE::ThirdPersonState*>(RE::PlayerCamera::GetSingleton()->currentState.get())) {
				tps->currentZoomOffset = currentZoom;
				tps->targetZoomOffset = targetZoom;
			}		
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
