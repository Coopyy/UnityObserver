// 0: Mono
// 1: IL2CPP
#define RUNTIME 0

// END OF CONFIGURATION

#include <cstdint>
#include <unordered_map>
#include <string>
#include <Windows.h>

#define MONO RUNTIME == 0
#define IL2CPP RUNTIME == 1

#ifdef _WIN64
#define CALLING_CONVENTION __fastcall
#elif _WIN32
#define CALLING_CONVENTION __cdecl
#endif

#if MONO
#define RUNTIME_DLL "mono-2.0-bdwgc.dll"
#elif IL2CPP
#define RUNTIME_DLL "GameAssembly.dll"
#else
#error "Invalid Runtime"
#endif

#define THIS reinterpret_cast<uintptr_t>(this)
#define STRINGIFY(x) #x
#define RUNTIME_EXPORT_FUNC(name, export_name, type, ...) \
	typedef type(CALLING_CONVENTION* t_##name)(__VA_ARGS__); \
	static t_##name Export_##name = nullptr; \
	if (!Export_##name) { \
		auto mod = Memory::GetModule(RUNTIME_DLL); \
		Export_##name = mod.GetExport<t_##name>(STRINGIFY(export_name)); \
	}

#pragma region Class Forward Decls
namespace Runtime {
	class Domain;
	class Assembly;
	class Image;
	class Class;
	class Method;
	class Field;
	class Type;
	class VTable;
}

namespace Types {
	class Object;

	template <typename T>
	class BoxedValue;
}
#pragma endregion

#pragma region Decls
namespace Logger {
	void Setup();
	void Cleanup();
	void Log(const char* fmt, ...);
	void LogError(const char* fmt, ...);
	void LogException(Types::Object* exception);
}

namespace Memory {
	class Module {
	private:
		uintptr_t _base;
		std::unordered_map<std::string, uintptr_t> _exports;
	public:
		Module(uintptr_t base);
		Module();

		uintptr_t GetBase();

		template <typename T>
		T GetExport(std::string name);
	};

	template <typename T>
	T Read(uintptr_t address);

	template <typename T>
	void Write(uintptr_t address, T value);

	Module GetModule(std::string name);
}

namespace Runtime {
	using namespace Types;

	class Method {
		void* GetThunk();
	public:
		const char* GetName();

		/**
		 * @brief Invokes the method using runtime invoke.
		 *
		 * @tparam T The return type of the method. Defaults to void if not specified.
		 * @param instance The instance on which to invoke the method. If null, the method will be invoked as a static method.
		 * @param args The arguments to pass to the method. Value and enum types should be passed as pointers to the value.
		 * Reference and string types should be passed as Object pointers.
		 * 
		 * @return The return value of the method. If the return type is a value type, it will be boxed.
		 */
		template <typename T = void, typename... Args>
		T Invoke(Object* instance, Args... args);

		/**
		 * @brief Invokes the method using unmanaged thunks.
		 *
		 * @tparam T The return type of the method. Defaults to void if not specified.
		 * @param instance The instance on which to invoke the method. If null, the method will be invoked as a static method.
		 * @param args The arguments to pass to the method. Non-primitive value types must be boxed, enums must be cast to their underlying type.
		 * Reference and string types should be passed as Object pointers.
		 * 
		 * @return The return value of the method. Value return types will be boxed. Primitives will not.
		 */
		template <typename T = void, typename... Args>
		T InvokeFast(Object* instance, Args... args);

		/**
		 * @brief Retrieves the thunk for the method with specified signature.
		 *
		 * @tparam T The return type of the method.
		 * @tparam Args The argument types of the method.
		 * @return A function pointer with the specified signature representing the method thunk.
		 * The first parameter is the instance on which to invoke the method, null if static.
		 * The last argument is a pointer to an Object pointer which will be set to the exception if one is thrown, null to not catch exceptions.
		 */
		template <typename T, typename... Args>
		T(CALLING_CONVENTION* GetThunk())(Args..., Object**);

		unsigned int GetToken();
		Class* GetClass();
	};

	class Field {
	public:
		const char* GetName();
		uintptr_t GetOffset();

		template <typename T>
		T Get(Object* instance);

		template <typename T>
		void Set(Object* instance, T& value);

		template <typename T>
		T Get(VTable* instance);

		template <typename T>
		void Set(VTable* instance, T value);
	};

	class VTable {
	public:
		Class* GetClass();
	};

	class Assembly {
	public:
		Image* GetImage();
		const char* GetName();
		Class* GetClass(const char* nameSpace, const char* name);
	};

	class Domain {
	public:
		static Domain* GetRootDomain();
		std::vector <Assembly*> GetAssemblies();
		Assembly* GetAssembly(const char* name);
	};

	class Type {
	public:
		Object* GetSystemType();
	};

	class Class {
		VTable* GetVTable();
		bool GetBitFieldValue(int index);
	public:
		Object* New();
		const char* GetName();
		Type* GetType();
		Class* GetParent();
		bool IsSubclassOf(Class* parent);
		bool IsValueType();
		Field* GetField(const char* name);
		Method* GetMethod(const char* name, int index = 0, bool checkParents = true);

		template <typename T>
		BoxedValue<T>* Box(T* address);

		template <typename T = void, typename... Args>
		T InvokeMethod(const char* name, Object* instance, Args... args);

		template <typename T>
		void SetFieldValue(const char* name, T value, Object* instance = nullptr);

		template <typename T>
		T GetFieldValue(const char* name, Object* instance = nullptr);
	};
}

namespace Types {
	class Object;
	class String;

	class Object {
	public:
		static Runtime::Class* StaticRuntimeClass();
		static Object* New();
		Runtime::Class* RuntimeClass();
		Object* GetType();
		bool IsInstanceOf(Runtime::Class* klass);
		String* ToString();

		template <typename T>
		T* As();

		template <typename T>
		void SetFieldValue(const char* name, T value);

		template <typename T>
		T GetFieldValue(const char* name);

		template <typename T = void, typename... Args>
		T InvokeMethod(const char* name, Args... args);
	};

	template <typename T>
	class BoxedValue : public Object {
		void* UnboxToPtr();
	public:
		T Unbox();
		String* ToString();

		template <typename T = void, typename... Args>
		T InvokeMethod(const char* name, Args... args);
	};

	class String : public Object {
	public:
		static Runtime::Class* StaticRuntimeClass();
		static String* New(const char* str);
		const wchar_t* ToWide();
		int GetLength();
		std::string ToCPP();
		bool operator==(const char* str);
		bool operator==(const std::string& str);
		bool operator==(String* str);
	};

	template <typename T>
	class GCHandle {
		T* _object;
		void* _handle;
	public:
		GCHandle(T* object, bool pinned = false);
		~GCHandle();
		T* Get();
	};
}
#pragma endregion

#pragma region Defs

namespace Logger {
	inline void Setup() {
		AllocConsole();
		freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(0)), "conin$", "r", static_cast<_iobuf*>(__acrt_iob_func(0)));
		freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(1)), "conout$", "w", static_cast<_iobuf*>(__acrt_iob_func(1)));
		freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(2)), "conout$", "w", static_cast<_iobuf*>(__acrt_iob_func(2)));
	}

	inline void Cleanup() {
		fclose(static_cast<_iobuf*>(__acrt_iob_func(0)));
		fclose(static_cast<_iobuf*>(__acrt_iob_func(1)));
		fclose(static_cast<_iobuf*>(__acrt_iob_func(2)));
		FreeConsole();
	}

	inline void Log(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		printf("\n");
	}

	inline void LogError(const char* fmt, ...) {
		printf("[!] ");
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		printf("\n");
	}
	void Logger::LogException(Types::Object* exception)
	{
		auto message = exception->InvokeMethod<Types::String*>("ToString");
		if (message)
			Logger::LogError("Exception: %s", message->ToCPP().c_str());
	}
}
namespace Memory {
	Module::Module(uintptr_t base) : _base(base) {}
	Module::Module() : _base(0) {}

	inline uintptr_t Module::GetBase() {
		return _base;
	}

	template <typename T>
	T Module::GetExport(std::string name) {
		if (_base == 0) {
			return nullptr;
		}

		if (_exports.find(name) != _exports.end()) {
			return reinterpret_cast<T>(_exports[name]);
		}

		uintptr_t address = (uintptr_t)GetProcAddress((HMODULE)_base, name.c_str());  // Note: Can change to getting export with export table
		if (address == 0) {
			Logger::LogError("Failed to get export: %s", name.c_str());
			return nullptr;
		}

		_exports[name] = address;
		return reinterpret_cast<T>(address);
	}

	template <typename T>
	T Read(uintptr_t address) {
		return *reinterpret_cast<T*>(address);
	}

	template <typename T>
	void Write(uintptr_t address, T value) {
		*reinterpret_cast<T*>(address) = value;
	}

	inline Module GetModule(std::string name) {
		static std::unordered_map<std::string, Module> _modules;
		if (_modules.find(name) != _modules.end()) {
			return _modules[name];
		}

		Module mod((uintptr_t)GetModuleHandleA(name.c_str()));  // Note: Can change to getting base with PEB
		if (mod.GetBase() == 0) {
			Logger::LogError("Failed to get module: %s", name.c_str());
			return {};
		}

		_modules[name] = mod;
		return mod;
	}
}

namespace Runtime {
#pragma region Method
	inline const char* Method::GetName() {

#if MONO
		RUNTIME_EXPORT_FUNC(MethodGetName, mono_method_get_name, const char*, Method*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(MethodGetName, il2cpp_method_get_name, const char*, Method*);
#endif

		return Export_MethodGetName(this);
	}

	template <typename T, typename... Args>
	inline T Method::Invoke(Object* instance, Args... args) {

#if MONO
		RUNTIME_EXPORT_FUNC(MethodInvoke, mono_runtime_invoke, Object*, Method*, Object*, void**, Object**);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(MethodInvoke, il2cpp_runtime_invoke, Object*, Method*, Object*, void**, Object**);
#endif
		Object* exception = nullptr;
		Object* result;
		if constexpr (sizeof...(Args) > 0) {
			void* params[] = { args... };
			result = Export_MethodInvoke(this, instance, params, &exception);
		}
		else
			result = Export_MethodInvoke(this, instance, nullptr, &exception);

		if (exception)
			Logger::LogException(exception);

		if constexpr (!std::is_same_v<T, void>)
			return static_cast<T>(result);
	}

	template <typename T, typename... Args>
	inline T Method::InvokeFast(Object* instance, Args... args) {
		auto thunk = GetThunk();
		if (!thunk) {
			if constexpr (std::is_same_v<T, void>)
				return;
			return T();
		}

		Object* exception = nullptr;

		static auto invoke = [&]<typename ... IArgs> (IArgs... invokeArgs) {
			if constexpr (std::is_same_v<T, void>) {
				reinterpret_cast<void(CALLING_CONVENTION*)(IArgs..., Object**)>(thunk)(invokeArgs..., &exception);
				if (exception)
					Logger::LogException(exception);
			}
			else {
				T result = reinterpret_cast<T(CALLING_CONVENTION*)(IArgs..., Object**)>(thunk)(invokeArgs..., &exception);
				if (exception)
					Logger::LogException(exception);
				return result;
			}
		};

		return instance ? invoke(instance, args...) : invoke(args...);
	}

	inline void* Method::GetThunk() {
#if MONO
		RUNTIME_EXPORT_FUNC(MethodGetThunk, mono_method_get_unmanaged_thunk, void*, Method*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(MethodGetThunk, il2cpp_method_get_unmanaged_thunk, void*, Method*);
#endif

		auto thunk = Export_MethodGetThunk(this);
		if (!thunk) {
			Logger::LogError("Failed to get method thunk: %s", GetName());
			return nullptr;
		}

		return thunk;
	}

	template <typename T, typename... Args>
	inline T(CALLING_CONVENTION* Method::GetThunk())(Args..., Object**) {
		return reinterpret_cast<T(CALLING_CONVENTION*)(Args..., Object**)>(GetThunk());
	}

	inline unsigned int Method::GetToken() {

#if MONO
		RUNTIME_EXPORT_FUNC(MethodGetToken, mono_method_get_token, unsigned int, Method*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(MethodGetToken, il2cpp_method_get_token, unsigned int, Method*);
#endif

		return Export_MethodGetToken(this);
	}

	inline Class* Method::GetClass() {

#if MONO
		RUNTIME_EXPORT_FUNC(MethodGetClass, mono_method_get_class, Class*, Method*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(MethodGetClass, il2cpp_method_get_class, Class*, Method*);
#endif

		return Export_MethodGetClass(this);

	}
#pragma endregion
#pragma region Field
	inline const char* Field::GetName() {

#if MONO
		RUNTIME_EXPORT_FUNC(FieldGetName, mono_field_get_name, const char*, Field*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(FieldGetName, il2cpp_field_get_name, const char*, Field*);

#endif

		return Export_FieldGetName(this);
	}

	inline uintptr_t Field::GetOffset() {
		static std::unordered_map<Field*, uintptr_t> _offsets;
		if (_offsets.find(this) != _offsets.end()) {
			return _offsets[this];
		}

#if MONO
		RUNTIME_EXPORT_FUNC(FieldGetOffset, mono_field_get_offset, uintptr_t, Field*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(FieldGetOffset, il2cpp_field_get_offset, uintptr_t, Field*);
#endif

		auto offset = Export_FieldGetOffset(this);
		_offsets[this] = offset;
		return offset;
	}

	template <typename T>
	inline T Field::Get(Object* instance) {
		T value;

#if MONO
		RUNTIME_EXPORT_FUNC(FieldGetValue, mono_field_get_value, void, Object*, Field*, void*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(FieldGetValue, il2cpp_field_get_value, void, Object*, Field*, void*);
#endif
		Export_FieldGetValue(instance, this, &value);
		return value;
	}

	template <typename T>
	inline void Field::Set(Object* instance, T& value) {

#if MONO
		RUNTIME_EXPORT_FUNC(FieldSetValue, mono_field_set_value, void, Object*, Field*, void*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(FieldSetValue, il2cpp_field_set_value, void, Object*, Field*, void*);
#endif
		Export_FieldSetValue(instance, this, &value);
	}

	template <typename T>
	inline T Field::Get(VTable* instance) {
		T value;

#if MONO
		RUNTIME_EXPORT_FUNC(FieldGetStaticValue, mono_field_static_get_value, void, VTable*, Field*, void*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(FieldGetStaticValue, il2cpp_field_static_get_value, void, VTable*, Field*, void*);
#endif

		Export_FieldGetStaticValue(instance, this, &value);
		return value;
	}

	template <typename T>
	inline void Field::Set(VTable* instance, T value) {

#if MONO
		RUNTIME_EXPORT_FUNC(FieldSetStaticValue, mono_field_static_set_value, void, VTable*, Field*, void*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(FieldSetStaticValue, il2cpp_field_static_set_value, void, VTable*, Field*, void*);
#endif

		Export_FieldSetStaticValue(instance, this, &value);

	}
#pragma endregion
#pragma region VTable
	inline Class* VTable::GetClass() {
		return Memory::Read<Class*>(THIS);
	}
#pragma endregion
#pragma region Assembly
	inline Image* Assembly::GetImage() {
#if MONO
		RUNTIME_EXPORT_FUNC(AssemblyGetImage, mono_assembly_get_image, Image*, Assembly*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(AssemblyGetImage, il2cpp_assembly_get_image, Image*, Assembly*);
#endif
		return Export_AssemblyGetImage(this);
	}
	inline const char* Assembly::GetName() {
#if MONO
		RUNTIME_EXPORT_FUNC(AssemblyGetName, mono_assembly_get_name, uintptr_t, Assembly*);

		auto _assemblyNamePtr = Export_AssemblyGetName(this);
		if (!_assemblyNamePtr) {
			return nullptr;
		}
		return Memory::Read<const char*>(_assemblyNamePtr);

#elif IL2CPP

		RUNTIME_EXPORT_FUNC(ImageGetName, il2cpp_image_get_name, const char*, Image*);

		return Export_ImageGetName(GetImage());
#endif
	}

	inline Class* Assembly::GetClass(const char* nameSpace, const char* name) {
		auto image = GetImage();
		if (!image) {
			return nullptr;
		}

#if MONO
		RUNTIME_EXPORT_FUNC(ImageGetClass, mono_class_from_name, Class*, Image*, const char*, const char*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(ImageGetClass, il2cpp_class_from_name, Class*, Image*, const char*, const char*);
#endif

		return Export_ImageGetClass(image, nameSpace, name);
	}
#pragma endregion
#pragma region Domain
	inline Domain* Domain::GetRootDomain() {

#if MONO
		RUNTIME_EXPORT_FUNC(GetDomain, mono_get_root_domain, Domain*);
		RUNTIME_EXPORT_FUNC(ThreadAttach, mono_thread_attach, void, Domain*);
		RUNTIME_EXPORT_FUNC(JITThreadAttach, mono_jit_thread_attach, void, Domain*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(GetDomain, il2cpp_domain_get, Domain*);
		RUNTIME_EXPORT_FUNC(ThreadAttach, il2cpp_thread_attach, void, Domain*);
#endif

		static Domain* rootDomain = nullptr;
		if (!rootDomain) {

			rootDomain = Export_GetDomain();
			Export_ThreadAttach(rootDomain);
#if MONO
			Export_JITThreadAttach(rootDomain);
#endif
			Logger::Log("UnityObserver initialized with Root Domain: %p", rootDomain);
		}

		return rootDomain;
	}


	inline std::vector <Assembly*> Domain::GetAssemblies() {
		std::vector<Assembly*> assemblies;

#if MONO
		RUNTIME_EXPORT_FUNC(DomainAssemblyForeach, mono_domain_assembly_foreach, void, Domain*, void(__fastcall * func)(void*, void*), void*);

		static auto callback = [](void* assembly, void* user_data) {
			auto assemblies = reinterpret_cast<std::vector<Assembly*>*>(user_data);
			assemblies->push_back(reinterpret_cast<Assembly*>(assembly));
			};

		Export_DomainAssemblyForeach(this, callback, &assemblies);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(DomainGetAssemblies, il2cpp_domain_get_assemblies, Assembly**, Domain*, size_t*);

		size_t size;
		auto _assemblies = DomainGetAssemblies(this, &size);
		for (size_t i = 0; i < size; i++)
			assemblies.push_back(_assemblies[i]);
#endif

		return assemblies;
	}

	inline Assembly* Domain::GetAssembly(const char* name) {
		static std::unordered_map<std::string, Assembly*> _assemblies;
		if (_assemblies.find(name) != _assemblies.end()) {
			return _assemblies[name];
		}

		for (const auto assembly : GetAssemblies()) {
			auto assemblyName = assembly->GetName();
			_assemblies[assemblyName] = assembly;
			if (strcmp(assemblyName, name) == 0) {
				return assembly;
			}
		}

		return nullptr;
	}
#pragma endregion
#pragma region Type
	inline Object* Type::GetSystemType() {

#if MONO
		RUNTIME_EXPORT_FUNC(TypeGetSystemType, mono_type_get_object, Object*, Domain*, Type*);
		return Export_TypeGetSystemType(Domain::GetRootDomain(), this);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(TypeGetSystemType, il2cpp_type_get_object, Object*, Type*);
		return Export_TypeGetSystemType(this);
#endif
	}
#pragma endregion
#pragma region Class
	inline VTable* Class::GetVTable() {
		static std::unordered_map<Class*, VTable*> _vtables;
		if (_vtables.find(this) != _vtables.end()) {
			return _vtables[this];
		}
#if MONO
		RUNTIME_EXPORT_FUNC(GetVTable, mono_class_vtable, VTable*, Domain*, Class*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(GetVTable, il2cpp_class_vtable, VTable*, Domain*, Class*);
#endif
		VTable* vtable = Export_GetVTable(Domain::GetRootDomain(), this);
		_vtables[this] = vtable;
		return vtable;
	}

	inline bool Class::GetBitFieldValue(int index) {
		return (Memory::Read<unsigned char>(THIS + 0x18) >> index) & 1;
	}

	inline Object* Class::New() {

#if MONO
		RUNTIME_EXPORT_FUNC(ObjectNew, mono_object_new, Object*, Domain*, Class*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(ObjectNew, il2cpp_object_new, Object*, Domain*, Class*);
#endif
		return Export_ObjectNew(Domain::GetRootDomain(), this);
	}

	inline const char* Class::GetName() {

#if MONO
		RUNTIME_EXPORT_FUNC(ClassGetName, mono_class_get_name, const char*, Class*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(ClassGetName, il2cpp_class_get_name, const char*, Class*);
#endif

		return Export_ClassGetName(this);
	}

	inline Type* Class::GetType() {

#if MONO
		RUNTIME_EXPORT_FUNC(ClassGetType, mono_class_get_type, Type*, Class*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(ClassGetType, il2cpp_class_get_type, Type*, Class*);
#endif

		return Export_ClassGetType(this);
	}

	inline Class* Class::GetParent() {

#if MONO
		RUNTIME_EXPORT_FUNC(ClassGetParent, mono_class_get_parent, Class*, Class*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(ClassGetParent, il2cpp_class_get_parent, Class*, Class*);
#endif

		return Export_ClassGetParent(this);
	}

	template <typename T>
	inline BoxedValue<T>* Class::Box(T* address) {

#if MONO
		RUNTIME_EXPORT_FUNC(Box, mono_value_box, BoxedValue<T>*, Domain*, Class*, void*);
		return Export_Box(Domain::GetRootDomain(), this, address);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(Box, il2cpp_value_box, BoxedValue<T>*, Class*, void*);
		return Export_Box(this, address);
#endif
	}

	inline bool Class::IsSubclassOf(Class* parent) {

#if MONO
		RUNTIME_EXPORT_FUNC(IsSubclass, mono_class_is_subclass_of, bool, Class*, Class*, bool);
		return Export_IsSubclass(this, parent, true);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(IsSubclassOf, il2cpp_class_is_subclass_of, bool, Class*, Class*, bool);
		return Export_IsSubclass(this, parent, true);
#endif
	}

	inline bool Class::IsValueType() {
		return GetBitFieldValue(1);
	}

	inline Field* Class::GetField(const char* name) {
		static std::unordered_map<Class*, std::unordered_map<std::string, Field*>> _fields;
		if (_fields.find(this) != _fields.end()) {
			auto& fields = _fields[this];
			if (fields.find(name) != fields.end()) {
				return fields[name];
			}
		}

#if MONO
		RUNTIME_EXPORT_FUNC(GetField, mono_class_get_field_from_name, Field*, Class*, const char*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(GetField, il2cpp_class_get_field_from_name, Field*, Class*, const char*);
#endif

		Field* field = Export_GetField(this, name);
		if (field) {
			_fields[this][name] = field;
		}

		return field;
	}

	inline Method* Class::GetMethod(const char* name, int index, bool checkParents) {
		static std::unordered_map<Class*, std::unordered_map<std::string, std::unordered_map<int, Method*>>> _methods;

		if (_methods.find(this) != _methods.end()) {
			auto& methods = _methods[this];
			if (methods.find(name) != methods.end()) {
				auto& overloads = methods[name];
				if (overloads.find(index) != overloads.end()) {
					return overloads[index];
				}
			}
		}

#if MONO
		RUNTIME_EXPORT_FUNC(GetMethod, mono_class_get_methods, Method*, Class*, uintptr_t*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(GetMethod, il2cpp_class_get_methods, Method*, Class*, uintptr_t*);
#endif
		Class* klass = this;
		while (klass) {
			uintptr_t iter{};
			int curIndex = index;
			while (Method* method = Export_GetMethod(klass, &iter)) {
				if (strcmp(method->GetName(), name) == 0) {
					if (curIndex == 0) {
						_methods[klass][name][index] = method;
						return method;
					}
					curIndex--;
				}
			}

			if (!checkParents) {
				break;
			}

			klass = klass->GetParent();
		}

		return nullptr;
	}

	template <typename T, typename... Args>
	inline T Class::InvokeMethod(const char* name, Object* instance, Args... args) {
		auto method = GetMethod(name);
		if (method) {
			return method->Invoke<T>(instance, args...);
		}

		return T();
	}

	template <typename T>
	inline void Class::SetFieldValue(const char* name, T value, Object* instance) {
		auto field = GetField(name);
		if (!field) {
			return;
		}

		if (!instance)
			field->Set(GetVTable(), value);
		else
			field->Set(instance, value);
	}

	template <typename T>
	inline T Class::GetFieldValue(const char* name, Object* instance) {
		auto field = GetField(name);
		if (!field) {
			return T();
		}

		if (!instance)
			return field->Get<T>(GetVTable());
		else
			return field->Get<T>(instance);
	}
#pragma endregion
}

namespace Types {
#pragma region Object
	inline Runtime::Class* Object::StaticRuntimeClass() {
		static Runtime::Class* _class = nullptr;
		if (_class) {
			return _class;
		}

#if MONO
		RUNTIME_EXPORT_FUNC(GetObjectClass, mono_get_object_class, Runtime::Class*);
		_class = Export_GetObjectClass();
#elif IL2CPP
		auto domain = Runtime::Domain::GetRootDomain();
		auto assembly = domain->GetAssembly("mscorlib");
		_class = assembly->GetClass("System", "Object");
#endif
		return _class;
	}

	inline Object* Object::New() {
		auto klass = StaticRuntimeClass();
		if (!klass) {
			return nullptr;
		}

		return klass->New();
	}

	inline Runtime::Class* Object::RuntimeClass() {
		auto vtable = Memory::Read<Runtime::VTable*>(THIS);
		if (!vtable) {
			return nullptr;
		}

		return vtable->GetClass();
	}

	inline Object* Object::GetType() {
		auto klass = RuntimeClass();
		if (!klass) {
			return nullptr;
		}

		auto type = klass->GetType();
		if (!type) {
			return nullptr;
		}

		return type->GetSystemType();
	}

	inline bool Object::IsInstanceOf(Runtime::Class* klass) {
		auto curClass = RuntimeClass();
		if (!curClass) {
			return false;
		}

		if (curClass == klass) {
			return true;
		}

		return curClass->IsSubclassOf(klass);
	}

	template <typename T>
	inline T* Object::As() {
		if (!IsInstanceOf(T::StaticRuntimeClass())) {
			return nullptr;
		}

		return reinterpret_cast<T*>(this);
	}

	template <typename T>
	inline void Object::SetFieldValue(const char* name, T value) {
		auto klass = RuntimeClass();
		if (!klass) {
			return;
		}

		klass->SetFieldValue(name, value, this);
	}

	template <typename T>
	inline T Object::GetFieldValue(const char* name) {
		auto klass = RuntimeClass();
		if (!klass) {
			return T();
		}

		return klass->GetFieldValue<T>(name, this);
	}

	template <typename T, typename... Args>
	inline T Object::InvokeMethod(const char* name, Args... args) {
		auto method = RuntimeClass()->GetMethod(name);
		if (method) {
			return method->Invoke<T>(this, args...);
		}

		return T();
	}

	inline String* Object::ToString() {
		return InvokeMethod<String*>("ToString");
	}
#pragma endregion
#pragma region BoxedValue
	template<typename T>
	inline void* BoxedValue<T>::UnboxToPtr()
	{
		if (!RuntimeClass()->IsValueType()) {
			Logger::LogError("Tried to unbox non-value type: %s", RuntimeClass()->GetName());
			return nullptr;
		}
#if MONO
		RUNTIME_EXPORT_FUNC(Unbox, mono_object_unbox, void*, Object*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(Unbox, il2cpp_object_unbox, void*, Object*);
#endif
		return Export_Unbox(this);
	}

	template<typename T>
	inline T BoxedValue<T>::Unbox() {
		auto unboxed = UnboxToPtr();
		if (!unboxed)
			return T();

		return Memory::Read<T>(reinterpret_cast<uintptr_t>(unboxed));
	}

	template<typename C>
	template<typename T, typename ...Args>
	inline T BoxedValue<C>::InvokeMethod(const char* name, Args ...args)
	{
		auto unboxed = UnboxToPtr();
		if (!unboxed) {
			return T();
		}

		auto method = RuntimeClass()->GetMethod(name);
		if (method) {
			return method->Invoke<T>(reinterpret_cast<Object*>(unboxed), nullptr, args...);
		}

		return T();
	}

	template<typename T>
	inline String* BoxedValue<T>::ToString() {
		return InvokeMethod<String*>("ToString");
	}

#pragma endregion
#pragma region String
	inline Runtime::Class* String::StaticRuntimeClass() {
		static Runtime::Class* _class = nullptr;
		if (_class)
			return _class;

		auto domain = Runtime::Domain::GetRootDomain();
		auto assembly = domain->GetAssembly("mscorlib");
		_class = assembly->GetClass("System", "String");

		return _class;
	}

	inline String* String::New(const char* str) {
#if MONO
		RUNTIME_EXPORT_FUNC(StringNew, mono_string_new, String*, Runtime::Domain*, const char*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(StringNew, il2cpp_string_new, String*, const char*);
#endif

		return Export_StringNew(Runtime::Domain::GetRootDomain(), str);
	}

	inline const wchar_t* String::ToWide() {

#if MONO
		RUNTIME_EXPORT_FUNC(StringGetChars, mono_string_chars, const wchar_t*, String*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(StringGetChars, il2cpp_string_chars, const wchar_t*, String*);
#endif

		return Export_StringGetChars(this);
	}

	inline int String::GetLength() {

#if MONO
		RUNTIME_EXPORT_FUNC(StringGetLength, mono_string_length, int, String*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(StringGetLength, il2cpp_string_length, int, String*);
#endif

		return Export_StringGetLength(this);
	}

	inline std::string String::ToCPP() {
		auto length = GetLength();
		auto chars = ToWide();
		std::string str;
		for (int i = 0; i < length; i++) {
			str += static_cast<char>(chars[i]);
		}
		return str;
	}

	inline bool String::operator==(const char* str) {
		return strcmp(ToCPP().c_str(), str) == 0;
	}

	inline bool String::operator==(const std::string& str) {
		return ToCPP() == str;
	}

	inline bool String::operator==(String* str) {
		return ToCPP() == str->ToCPP();
	}
#pragma endregion
#pragma region CGHandle
	template<typename T>
	inline GCHandle<T>::GCHandle(T* object, bool pinned) : _object(object) {

#if MONO
		RUNTIME_EXPORT_FUNC(GCHandleNew, mono_gchandle_new_v2, void*, T*, bool);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(GCHandleNew, il2cpp_gchandle_new, void*, T*, bool);
#endif

		_handle = Export_GCHandleNew(object, pinned);
	}

	template<typename T>
	inline GCHandle<T>::~GCHandle() {

#if MONO
		RUNTIME_EXPORT_FUNC(GCHandleFree, mono_gchandle_free_v2, void, void*);
#elif IL2CPP
		RUNTIME_EXPORT_FUNC(GCHandleFree, il2cpp_gchandle_free, void, void*);
#endif

		Export_GCHandleFree(_handle);
	}

	template<typename T>
	inline T* GCHandle<T>::Get() {
		return _object;
	}
#pragma endregion
}

#pragma endregion
