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

		auto localPos = boxedPosition->Unbox();
		Logger::Log("Position: %f %f %f", localPos.x, localPos.y, localPos.z);

		// Test instance value methods
		Logger::Log("Player ToString: %s", plr->ToString()->ToCPP().c_str());

		Logger::Log("Position ToString: %s", boxedPosition->ToString()->ToCPP().c_str());

		BoxedValue<float>* magnitude = boxedPosition->InvokeMethod<BoxedValue<float>*>("get_magnitude");
		Logger::Log("Position get_magnitude: %f", magnitude->Unbox());

		// Test static value methods
		Vector3 somePos = { 100, 2, 3 };

		auto Vector3Class = Domain::GetRootDomain()->GetAssembly("UnityEngine.CoreModule")->GetClass("UnityEngine", "Vector3");
		auto Vector3Distance = Vector3Class->GetMethod("Distance");

		auto distance = Vector3Distance->Invoke<BoxedValue<float>*>(nullptr, &somePos, &localPos)->Unbox();
		auto distanceFast = Vector3Distance->InvokeFast<float>(nullptr, Vector3Class->Box<Vector3>(&somePos), Vector3Class->Box<Vector3>(&localPos));
		auto distanceFaster = Vector3Distance->InvokeUnsafe<float>(nullptr, &somePos, &localPos);

		Logger::Log("Vector3 Distance: %f", distance);
		Logger::Log("Vector3 Distance Fast: %f", distanceFast);
		Logger::Log("Vector3 Distance Faster: %f", distanceFaster);
	}

	auto DebugLog = Domain::GetRootDomain()->GetAssembly("UnityEngine.CoreModule")->GetClass("UnityEngine", "Debug")->GetMethod("Log");

	DebugLog->Invoke(nullptr, String::New("Invoke"));
	DebugLog->InvokeFast(nullptr, String::New("InvokeFast"));


	auto DebugLogThunk = DebugLog->GetThunk<void, String*>();
	Object* exception = nullptr;

	DebugLogThunk(String::New("InvokeThunk"), &exception);

	if (exception)
		Logger::LogException(exception);

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

