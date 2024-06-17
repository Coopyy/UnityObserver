#include "unity_observer.h"
#include <chrono>
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

		Logger::Log("Player ToString: %s", plr->ToString()->ToCPP().c_str());

		// Player.player.transform
		auto transform = plr->InvokeMethod<Object*>("get_transform");
		Logger::Log("Transform 0x%p", transform);

		// Player.player.transform.position
		auto localPos = transform->InvokeMethod<BoxedValue<Vector3>*>("get_position")->Unbox();

		// Test instance value methods
		Logger::Log("Player Position: %f %f %f", localPos.x, localPos.y, localPos.z);
		Logger::Log("Player Position ToString: %s", transform->ToString()->ToCPP().c_str());

		// Test static value methods
		Logger::Log("----------------------");

		Vector3 somePos = { 100, 2, 3 };

		auto Vector3Class = Domain::GetRootDomain()->GetAssembly("UnityEngine.CoreModule")->GetClass("UnityEngine", "Vector3");
		auto Vector3Distance = Vector3Class->GetMethod("Distance");

		// make sure the method is compiled and thunks are generated
		Vector3Distance->GetCompiled<void>();
		Vector3Distance->GetThunk<void>();

		auto boxedSome = Vector3Class->Box<Vector3>(&somePos);
		auto boxedLocal = Vector3Class->Box<Vector3>(&localPos);

		auto start = std::chrono::high_resolution_clock::now();
		auto distance = Vector3Distance->Invoke<BoxedValue<float>*>(nullptr, &somePos, &localPos)->Unbox();
		auto end = std::chrono::high_resolution_clock::now();
		Logger::Log("Vector3 Distance: %f - %f ms", distance, std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f);

		start = std::chrono::high_resolution_clock::now();
		auto distanceFast = Vector3Distance->InvokeFast<float>(nullptr, boxedSome, boxedLocal);
		end = std::chrono::high_resolution_clock::now();
		Logger::Log("Vector3 Distance Fast: %f - %f ms", distanceFast, std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f);

		start = std::chrono::high_resolution_clock::now();
		auto distanceFaster = Vector3Distance->InvokeUnsafe<float>(nullptr, &somePos, &localPos);
		end = std::chrono::high_resolution_clock::now();
		Logger::Log("Vector3 Distance Faster: %f - %f ms", distanceFaster, std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f);

		Logger::Log("----------------------");

		auto ts = Vector3Class->InvokeMethod<String*>("ToString", boxedSome);
		Logger::Log("ToString from class: %s", ts->ToCPP().c_str());

		ts = boxedSome->ToString();
		Logger::Log("ToString from boxed instance: %s", ts->ToCPP().c_str());
	}

	auto DebugLog = Domain::GetRootDomain()->GetAssembly("UnityEngine.CoreModule")->GetClass("UnityEngine", "Debug")->GetMethod("Log");

	DebugLog->Invoke(nullptr, String::New("Invoke"));
	DebugLog->InvokeFast(nullptr, String::New("InvokeFast"));
	DebugLog->InvokeUnsafe(nullptr, String::New("InvokeUnsafe"));

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

