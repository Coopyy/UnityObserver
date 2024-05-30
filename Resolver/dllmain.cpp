#include "unity_observer.h"

using namespace Runtime;

struct Vector3 {
	float x, y, z;
};

DWORD Main(LPVOID lpParam) {

	Logger::Setup();

	// Player.player
	auto plr = Domain::GetRootDomain()->GetAssembly("Assembly-CSharp")->GetClass("SDG.Unturned", "Player")->GetFieldValue<Object*>("_player");
	Logger::Log("Player: 0x%p", plr);

	if (plr) {

		// Player.player.transform
		auto transform = plr->InvokeMethod<Object*>("get_transform");
		Logger::Log("Transform 0x%p", transform);

		// Player.player.transform.position
		auto boxedPosition = transform->InvokeMethod<BoxedValue<Vector3>*>("get_position");
		Logger::Log("Boxed Position 0x%p", boxedPosition);

		auto pos = boxedPosition->Unbox();
		Logger::Log("Position: %f %f %f", pos.x, pos.y, pos.z);

		// Test instance value methods
		Logger::Log("Player ToString: %s", plr->ToString()->ToCPP().c_str());

		Logger::Log("Position ToString: %s", boxedPosition->ToString()->ToCPP().c_str());

		BoxedValue<float>* magnitdude = boxedPosition->InvokeMethod<BoxedValue<float>*>("get_magnitude");
		Logger::Log("Position get_magnitude: %f", magnitdude->Unbox());

		// Test static value methods
		Vector3 v1 = { 100, 2, 3 };

		auto vector3Class = Domain::GetRootDomain()->GetAssembly("UnityEngine.CoreModule")->GetClass("UnityEngine", "Vector3");
		auto distance = vector3Class->InvokeMethod<BoxedValue<float>*>("Distance", nullptr, &v1, &pos);

		Logger::Log("Vector3 Distance: %f", distance->Unbox());
	}

	auto DebugLog = Domain::GetRootDomain()->GetAssembly("UnityEngine.CoreModule")->GetClass("UnityEngine", "Debug")->GetMethod("Log");

	DebugLog->Invoke(nullptr, String::New("Invoke"));
	DebugLog->InvokeFast(nullptr, String::New("InvokeFast"));

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

