#include "unity_observer.h"

DWORD Main(LPVOID lpParam) {

	Logger::Setup();

	Logger::Log("Root domain: %p\n", Runtime::Domain::GetRootDomain());

	auto assembly = Runtime::Domain::GetRootDomain()->GetAssembly("Assembly-CSharp");
	Logger::Log("Assembly-CSharp: 0x%p\n", assembly);

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

