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

	//auto string = String::New("Hello World!");
	//Logger::Log("String: 0x%p", string);

	auto acs = Domain::GetRootDomain()->GetAssembly("vulp.gg");
	auto ChatManager = acs->GetClass("FurFrags.Logging", "GameConsole");

	Logger::Log("ChatManager: 0x%p", ChatManager);

	int test = 2;
	auto string = String::New("Hello Wo1rld!");

	auto sendChat = ChatManager->GetMethod("Log");

	RUNTIME_EXPORT_FUNC(MethodInvoke, mono_runtime_invoke, void*, Method*, Object*, void**, void**);
	RUNTIME_EXPORT_FUNC(MethodGetThunk, mono_method_get_unmanaged_thunk, void*, Method*);

	void* args[2] = { string, &test };

	Export_MethodInvoke(sendChat, nullptr, args, nullptr);


	//auto fn = reinterpret_cast<void(*)(void*, int, void*)>(Export_MethodGetThunk(sendChat));

	Object* exception = nullptr;

	
	//fn(string, test, &exception);

	if (exception) {
		Logger::Log("Exception: %s", exception->ToString()->ToCPP().c_str());
	}

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

