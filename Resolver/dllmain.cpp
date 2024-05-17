#include "unity_observer.h"

DWORD Main(LPVOID lpParam) {

	Logger::Setup();

	Logger::Log("Root domain: %p\n", Runtime::Domain::GetRootDomain());

	auto assembly = Runtime::Domain::GetRootDomain()->GetAssembly("Assembly-CSharp");
	Logger::Log("Assembly-CSharp: 0x%p\n", assembly);

	auto player = assembly->GetClass("SDG.Unturned", "Player");
	Logger::Log("Player Class: 0x%p\n", player);

	auto playerType = player->GetType();
	Logger::Log("Player Type: 0x%p\n", playerType);

	auto playerSystemType = playerType->GetSystemType();
	Logger::Log("Player System Type: 0x%p\n", playerSystemType);

	const auto assemblies = Runtime::Domain::GetRootDomain()->GetAssemblies();
	Logger::Log("Found %d assemblies\n", assemblies.size());

	for (const auto assembly : assemblies) {
		Logger::Log("Assembly: %s\n", assembly->GetName());
	}

	while (!GetAsyncKeyState(VK_END)) Sleep(1000);

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

