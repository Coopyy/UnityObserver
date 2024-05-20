#include "unity_observer.h"

DWORD Main(LPVOID lpParam) {

	Logger::Setup();

	Logger::Log("Root domain: %p\n", Runtime::Domain::GetRootDomain());

	auto playerClass = Runtime::Domain::GetRootDomain()->GetAssembly("Assembly-CSharp")->GetClass("SDG.Unturned", "Player");

	auto plr = playerClass->GetFieldValue<Types::Object*>("_player");

	plr->GetFieldValue<Types::Object*>("_life")->SetFieldValue("_health", 100);

	Logger::Log("Player: 0x%p\n", plr);

	auto GetNetId = playerClass->GetMethod("GetNetId");
    Logger::Log("GetNetId: 0x%p\n", GetNetId);
	struct netID {
		unsigned int id;
	};
	auto netId = GetNetId->Invoke<Types::BoxedValue<netID>*>(plr);
	Logger::Log("NetId: %p\n", netId);

	while (!GetAsyncKeyState(VK_END)) Sleep(100);

	Logger::Cleanup();

	FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), EXIT_SUCCESS);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		const HANDLE hThread = CreateThread(nullptr, NULL, Main, hModule, NULL, nullptr);
		if (hThread)
			CloseHandle(hThread);
	}
	return TRUE;
}

