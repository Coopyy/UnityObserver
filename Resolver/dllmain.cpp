#include "unity_observer.h"

DWORD Main(LPVOID lpParam) {

	Logger::Setup();

	// Tests
	auto mod = Memory::GetModule("mono-2.0-bdwgc.dll");
	auto base = mod.GetBase();
	Logger::Log("mono-2.0-bdwgc.dll: 0x%p\n", base);

	auto mono_get_root_domain = mod.GetExport<uintptr_t(__cdecl*)()>("mono_get_root_domain");
	Logger::Log("mono_get_root_domain: 0x%p\n", mono_get_root_domain());

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

