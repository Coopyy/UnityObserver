#include "../unity_observer.h"

// store original function pointers
using Update_t = void (__stdcall*)(void* thisPtr);
Update_t ogUpdate = nullptr;

// hook function
void __stdcall hkUpdate(void* thisPtr) {
	Logger::Log("Update called");

	ogUpdate(thisPtr);
}

inline void hookUpdate() {
	// get unityplayer.dll module handle
	auto hModule = Memory::GetModule("UnityPlayer.dll");
	Logger::Log("UnityPlayer.dll module handle: %p", hModule.GetBase());

	void** behaviorManagerVTable = (void**)(hModule.GetBase() + 0x172FA10);
	Logger::Log("BehaviorManager vtable: %p", behaviorManagerVTable);

	if (!behaviorManagerVTable)
	{
		Logger::Log("Failed to get UpdateManager vtable");
		return;
	}

	/*
	* BehaviourManager vtable:
	* 0: destructor
	* 1: update
	*/

	void** updateEntry = behaviorManagerVTable + 1;
	Logger::Log("Update function entry: %p", updateEntry);

	ogUpdate = (Update_t)(*updateEntry);
	Logger::Log("Update function: %p", ogUpdate);

	// hook update function
	DWORD oldProtect;
	VirtualProtect(updateEntry, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*updateEntry = &hkUpdate;
	VirtualProtect(updateEntry, sizeof(void*), oldProtect, &oldProtect);
}

inline void hookGUI() {




}

inline void hooksTest() {
	Logger::Log("Testing hooks");

	hookUpdate();
	hookGUI();
}