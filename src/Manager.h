#pragma once

class AutoSandboxHandler :
	public ISingleton<AutoSandboxHandler>,
	public RE::PlayerInputHandler
{
public:
	~AutoSandboxHandler() override = default;  // 00

	bool CanProcess(RE::InputEvent* a_event) override;                                      // 01
	void ProcessButton(RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data) override;  // 04

	void Register();

	bool IsAutoSandboxing() const;
	bool CanSandbox() const;
	bool StartSandbox();
	void StopSandbox();

	void ResetSandboxCheck();

	RE::TESPackage* GetPackage();

private:
	// members
	bool            autoSandBoxing{ false };
	bool            sandboxCheckFailed{ false };
	bool            isInFirstPerson{ false };
	bool            improvedCameraInstalled{ false };
	RE::TESPackage* exteriorPackage{ nullptr };
	RE::TESPackage* interiorPackage{ nullptr };
	RE::BGSKeyword* furnitureForceThirdPerson{ nullptr };
	RE::BGSKeyword* furnitureForceFirstPerson{ nullptr };
	RE::BGSKeyword* furnitureSpecial{ nullptr };

};
