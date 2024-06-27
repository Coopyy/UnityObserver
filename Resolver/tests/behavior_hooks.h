#include "../unity_observer.h"

// messy playground for unity hooks

using namespace Runtime;

#pragma region Update
// store original function pointers
using Update_t = void(__stdcall*)(void* thisPtr);
inline Update_t ogUpdate = nullptr;
inline void** updateEntry = nullptr;

inline Memory::Module GetUnityPlayerModule() {
	static Memory::Module unityPlayerModule = Memory::GetModule("UnityPlayer.dll");
	return unityPlayerModule;
}

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
	void** behaviorManagerVTable = (void**)(GetUnityPlayerModule().GetBase() + 0x172FA10);
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
#pragma endregion

#pragma region Gui

class IMGUIModule;
class ObjectGUIState;
struct PPtr {
	int m_InstanceID;
};
struct ScriptingMethodPtr {
	void* m_BackendMethod;
};

using DoGUI_t = bool(__stdcall*)(IMGUIModule* thisPtr, int displayIndex, ObjectGUIState* objectGUIState, int layoutType, int skin, ScriptingMethodPtr method, PPtr behaviourPPtr);
inline DoGUI_t ogDoGUI = nullptr;
inline void** doGUIEntry = nullptr;
inline int64_t lastFrameCount = 0;

bool __stdcall hkDoGUI(IMGUIModule* thisPtr, int displayIndex, ObjectGUIState* objectGUIState, int layoutType, int skin, ScriptingMethodPtr method, PPtr behaviourPPtr) {

	static int64_t* curFramePtr = nullptr;
	if (!curFramePtr) {
		auto timePtr = *(uintptr_t*)((GetUnityPlayerModule().GetBase() + 0x1ADC670) + 0x8 * 7);
		curFramePtr = (int64_t*)(timePtr + 0xC8);
	}

	int64_t currentFrame = *curFramePtr;
	if (currentFrame != lastFrameCount) {
		Logger::Log("Frame count: %d", currentFrame);

		static Class* GUILayout = Class::Find("UnityEngine.GUILayout");
		static Method* boxMethod = GUILayout->GetMethod("Box", 1);
		boxMethod->Invoke(nullptr, String::New("test"), nullptr);

		// todo: set gui depth, throws exception

		lastFrameCount = currentFrame;
	}

	return ogDoGUI(thisPtr, displayIndex, objectGUIState, layoutType, skin, method, behaviourPPtr);
}

// todo: figure out best way to hook GUI
// IMGUIModule::MonoBehaviourDoGUI can be ptr swapped, but we'd need to check current frame count to run once
// would also have to kinda rebuild some of the last method in the callstack

// --gui behavior callstack--
// GUIManager::DoGUIEvent(GUIManager *this, InputEvent *eventToSend, bool frontToBack)
// MonoBehaviourDoGUI(void *beh, __int64 layoutType, __int64 skin, __int64 displayIndex)
// MonoBehaviour::DoGUI(MonoBehaviour *this, unsigned int layoutType, int skin, unsigned int displayIndex)
// IMGUIModule::MonoBehaviourDoGUI(IMGUIModule* this, int displayIndex, ObjectGUIState* objectGUIState, int layoutType, int skin, ScriptingMethodPtr method, PPtr<MonoBehaviour> behaviourPPtr)
// MonoBehaviourDoGUI(int displayIndex, ObjectGUIState* objectGUIState, int layoutType, int skin, ScriptingMethodPtr method, int behaviourPPtr)
// (managed OnGUI())
inline void hookGUI() {
	auto IMGUIVTable = (void**)(GetUnityPlayerModule().GetBase() + 0x17EEA10);
	Logger::Log("IMGUI vtable: %p", IMGUIVTable);

	if (!IMGUIVTable)
	{
		Logger::Log("Failed to get IMGUI vtable");
		return;
	}

	/*
	* IMGUI vtable:
	* 0: doGUI
	* other shit i cba
	*/

	doGUIEntry = IMGUIVTable + 0;
	Logger::Log("DoGUI function entry: %p", doGUIEntry);

	ogDoGUI = (DoGUI_t)(*doGUIEntry);
	Logger::Log("DoGUI function: %p", ogDoGUI);

	// hook gui function
	DWORD oldProtect;
	VirtualProtect(doGUIEntry, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*doGUIEntry = &hkDoGUI;
	VirtualProtect(doGUIEntry, sizeof(void*), oldProtect, &oldProtect);
}
#pragma endregion

inline void hooksTest() {
	Logger::Log("Testing hooks");

	//hookUpdate();
	hookGUI();
}

inline void hooksCleanup() {
	Logger::Log("Cleaning up hooks");
	if (updateEntry && ogUpdate)
	{
		DWORD oldProtect;
		VirtualProtect(updateEntry, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
		*updateEntry = ogUpdate;
		VirtualProtect(updateEntry, sizeof(void*), oldProtect, &oldProtect);
	}

	if (doGUIEntry && ogDoGUI) {
		DWORD oldProtect;
		VirtualProtect(doGUIEntry, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
		*doGUIEntry = ogDoGUI;
		VirtualProtect(doGUIEntry, sizeof(void*), oldProtect, &oldProtect);
	}
}