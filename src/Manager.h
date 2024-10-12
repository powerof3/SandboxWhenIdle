#pragma once

class AutoSandboxHandler :
	public ISingleton<AutoSandboxHandler>,
	public RE::BSTEventSink<RE::InputEvent*>,
	public RE::BSTEventSink<RE::TESLoadGameEvent>
{
public:
	void Register();

	bool IsAutoSandboxing() const;
	bool CanSandbox() const;
	bool StartSandbox();
	void StopSandbox();

	void ResetSandboxCheck();

	void UpdateCameraForSandbox(bool a_enable);
	void SetHeadTracking(bool a_enable);

	RE::TESPackage* GetPackage();

private:
	RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;
	RE::BSEventNotifyControl ProcessEvent(const RE::TESLoadGameEvent* a_evn, RE::BSTEventSource<RE::TESLoadGameEvent>*) override;
	
	// members
	bool            autoSandBoxing{ false };
	bool            sandboxCheckFailed{ false };
	bool            registeredForInput{ false };
	bool            isInFirstPerson{ false };
	bool            headTrackingState{ false };
	bool            isNPCMode{ false };
	bool            improvedCameraInstalled{ false };
	float           currentZoom;
	float           targetZoom;
	RE::TESPackage* exteriorPackage{ nullptr };
	RE::TESPackage* interiorPackage{ nullptr };
	RE::BGSKeyword* furnitureForceThirdPerson{ nullptr };
	RE::BGSKeyword* furnitureForceFirstPerson{ nullptr };
	RE::BGSKeyword* furnitureSpecial{ nullptr };
};
