#include "../unity_observer.h"
using namespace Runtime;
// store original function pointers
using Update_t = void (__stdcall*)(void* thisPtr);
inline Update_t ogUpdate = nullptr;
inline void** updateEntry = nullptr;

// hook function
void __stdcall hkUpdate(void* thisPtr) {

	static Class* UnityEngine_Time = nullptr;
	if (!UnityEngine_Time) {
		UnityEngine_Time = Class::Find("UnityEngine.Time");
		Logger::Log("UnityEngine.Time: %p", UnityEngine_Time);
	}

	int frameCount = UnityEngine_Time->InvokeMethod<int>("get_frameCount", nullptr);
	Logger::Log("frameCount: %d", frameCount);

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

	updateEntry = behaviorManagerVTable + 1;
	Logger::Log("Update function entry: %p", updateEntry);

	ogUpdate = (Update_t)(*updateEntry);
	Logger::Log("Update function: %p", ogUpdate);

	// hook update function
	DWORD oldProtect;
	VirtualProtect(updateEntry, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*updateEntry = &hkUpdate;
	VirtualProtect(updateEntry, sizeof(void*), oldProtect, &oldProtect);
}

// todo: figure out best way to hook GUI

// --gui behavior callstack--
// GUIManager::DoGUIEvent(GUIManager *this, InputEvent *eventToSend, bool frontToBack)
// MonoBehaviourDoGUI(void *beh, __int64 layoutType, __int64 skin, __int64 displayIndex)
// MonoBehaviour::DoGUI(MonoBehaviour *this, unsigned int layoutType, int skin, unsigned int displayIndex)
// IMGUIModule::MonoBehaviourDoGUI(IMGUIModule* this, int displayIndex, ObjectGUIState* objectGUIState, int layoutType, int skin, ScriptingMethodPtr method, PPtr<MonoBehaviour> behaviourPPtr)
// MonoBehaviourDoGUI(int displayIndex, ObjectGUIState* objectGUIState, int layoutType, int skin, ScriptingMethodPtr method, int behaviourPPtr)
// (managed OnGUI())
inline void hookGUI() {

}

inline void hooksTest() {
	Logger::Log("Testing hooks");

	hookUpdate();
	hookGUI();
}

inline void hooksCleanup() {
	Logger::Log("Cleaning up hooks");

	DWORD oldProtect;
	VirtualProtect(updateEntry, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*updateEntry = ogUpdate;
	VirtualProtect(updateEntry, sizeof(void*), oldProtect, &oldProtect);
}