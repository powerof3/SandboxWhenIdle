#include "Manager.h"

bool AutoSandboxHandler::CanProcess(RE::InputEvent* a_event)
{
	return IsAutoSandboxing() && a_event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton;
}

void AutoSandboxHandler::ProcessButton(RE::ButtonEvent* a_event, [[maybe_unused]] RE::PlayerControlsData* a_data)
{
	if (a_event && a_event->IsDown()) {
		if (autoSandBoxing) {
			StopSandbox();
		}
	}
}

void AutoSandboxHandler::Register()
{
	RE::PlayerControls::GetSingleton()->handlers.emplace_back(this);

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
	if (sandboxCheckFailed) {
		return false;
	}

	auto player = RE::PlayerCharacter::GetSingleton();
	if (!player->currentProcess) {
		return false;
	}

	if (player->IsInCombat()) {
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
				isInFirstPerson = !improvedCameraInstalled && RE::PlayerCamera::GetSingleton()->IsInFirstPerson(); // don't switch if IC is installed

				if (isInFirstPerson){
					RE::PlayerCamera::GetSingleton()->PushCameraState(RE::CameraState::kThirdPerson);
					if (auto tps = skyrim_cast<RE::ThirdPersonState*>(RE::PlayerCamera::GetSingleton()->currentState.get())) {
						// default tps is too zoomed in
						tps->currentZoomOffset = 0.8f;
						tps->targetZoomOffset = 0.8f;
					}
				}		

				auto player = RE::PlayerCharacter::GetSingleton();
				player->DrawWeaponMagicHands(false);
				player->SetAIDriven(true);
				player->currentProcess->SetRunOncePackage(nullptr, player);
				player->currentProcess->ComputeLastTimeProcessed();
				player->PutCreatedPackage(package, false, false);

				RE::SendHUDMessage::PushHUDMode("AutoVanity");
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

	if (isInFirstPerson) {
		RE::PlayerCamera::GetSingleton()->ForceFirstPerson();
	}

	RE::PlayerCamera::GetSingleton()->idleTimer = 0.0f;
	RE::SendHUDMessage::PopHUDMode("AutoVanity");
}

void AutoSandboxHandler::ResetSandboxCheck()
{
	sandboxCheckFailed = false;
}

RE::TESPackage* AutoSandboxHandler::GetPackage()
{
	auto parentCell = RE::PlayerCharacter::GetSingleton()->parentCell;
	return parentCell && parentCell->IsInteriorCell() ? interiorPackage : exteriorPackage;
}
