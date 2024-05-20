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
	}

	inline void LogError(const char* fmt, ...) {
		printf("[!] ");
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
}

namespace Memory {
	class Module {
	private:
		uintptr_t _base;
		std::unordered_map<std::string, uintptr_t> _exports;
	public:
		Module(uintptr_t base) : _base(base) {}
		Module() : _base(0) {}

		inline uintptr_t GetBase() const {
			return _base;
		}

		template <typename T>
		T GetExport(std::string name) {
			if (_base == 0) {
				return nullptr;
			}

			if (_exports.find(name) != _exports.end()) {
				return reinterpret_cast<T>(_exports[name]);
			}

			uintptr_t address = (uintptr_t)GetProcAddress((HMODULE)_base, name.c_str());  // Note: Can change to getting export with export table
			if (address == 0) {
				Logger::LogError("Failed to get export: %s\n", name.c_str());
				return nullptr;
			}

			_exports[name] = address;
			return reinterpret_cast<T>(address);
		}
	};

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
			Logger::LogError("Failed to get module: %s\n", name.c_str());
			return {};
		}

		_modules[name] = mod;
		return mod;
	}
}

namespace Types {
	class Object;

	template <typename T>
	class BoxedValue;
}

namespace Runtime {
	using namespace Types;

	class Domain;
	class Assembly;
	class Image;
	class Class;
	class Method;
	class Field;
	class Type;
	class VTable;

	class Method {
	public:
		inline const char* GetName() {

#if MONO
			RUNTIME_EXPORT_FUNC(MethodGetName, mono_method_get_name, const char*, Method*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(MethodGetName, il2cpp_method_get_name, const char*, Method*);
#endif

			return Export_MethodGetName(this);
		}

		template <typename T = void, typename... Args>
		inline T Invoke(Object* instance, Args... args) {

#if MONO
			RUNTIME_EXPORT_FUNC(MethodInvoke, mono_runtime_invoke, T, Method*, Object*, void**, void**);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(MethodInvoke, il2cpp_runtime_invoke, T, Method*, Object*, void**, void**);
#endif

			if constexpr (sizeof...(Args) > 0) {
				void* params[] = { &args... };
				return Export_MethodInvoke(this, instance, params, nullptr);
			}
			else
				return Export_MethodInvoke(this, instance, nullptr, nullptr);
		}
	};

	class Field {
	public:

		inline const char* GetName() {

#if MONO
			RUNTIME_EXPORT_FUNC(FieldGetName, mono_field_get_name, const char*, Field*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(FieldGetName, il2cpp_field_get_name, const char*, Field*);

#endif

			return Export_FieldGetName(this);
		}

		inline uintptr_t GetOffset() {
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
		inline T Get(Object* instance) {
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
		inline void Set(Object* instance, T& value) {

#if MONO
			RUNTIME_EXPORT_FUNC(FieldSetValue, mono_field_set_value, void, Object*, Field*, void*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(FieldSetValue, il2cpp_field_set_value, void, Object*, Field*, void*);
#endif
			Export_FieldSetValue(instance, this, &value);
		}

		template <typename T>
		inline T Get(VTable* instance) {
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
		inline void Set(VTable* instance, T value) {

#if MONO
			RUNTIME_EXPORT_FUNC(FieldSetStaticValue, mono_field_static_set_value, void, VTable*, Field*, void*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(FieldSetStaticValue, il2cpp_field_static_set_value, void, VTable*, Field*, void*);
#endif

			Export_FieldSetStaticValue(instance, this, &value);

		}
	};

	class VTable {
	public:
		inline Class* GetClass() {
			return Memory::Read<Class*>(THIS);
		}
	};

	class Assembly {
		inline Image* GetImage() {
#if MONO
			RUNTIME_EXPORT_FUNC(AssemblyGetImage, mono_assembly_get_image, Image*, Assembly*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(AssemblyGetImage, il2cpp_assembly_get_image, Image*, Assembly*);
#endif
			return Export_AssemblyGetImage(this);
		}
	public:
		inline const char* GetName() {
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

		inline Class* GetClass(const char* nameSpace, const char* name) {
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
	};

	class Domain {
	public:

		static Domain* GetRootDomain() {

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
				Logger::Log("UnityObserver initialized with Root Domain: %p\n", rootDomain);
			}

			return rootDomain;
		}


		inline std::vector <Assembly*> GetAssemblies() {
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

		inline Assembly* GetAssembly(const char* name) {
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
	};

	class Type {
	public:
		inline Object* GetSystemType() {

#if MONO
			RUNTIME_EXPORT_FUNC(TypeGetSystemType, mono_type_get_object, Object*, Domain*, Type*);
			return Export_TypeGetSystemType(Domain::GetRootDomain(), this);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(TypeGetSystemType, il2cpp_type_get_object, Object*, Type*);
			return Export_TypeGetSystemType(this);
#endif
		}
	};

	class Class {
		inline VTable* GetVTable() {
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
	public:
		inline Object* New() {

#if MONO
			RUNTIME_EXPORT_FUNC(ObjectNew, mono_object_new, Object*, Domain*, Class*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(ObjectNew, il2cpp_object_new, Object*, Domain*, Class*);
#endif
			return Export_ObjectNew(Domain::GetRootDomain(), this);
		}

		inline const char* GetName() {

			#if MONO
			RUNTIME_EXPORT_FUNC(ClassGetName, mono_class_get_name, const char*, Class*);
			#elif IL2CPP
			RUNTIME_EXPORT_FUNC(ClassGetName, il2cpp_class_get_name, const char*, Class*);
			#endif

			return Export_ClassGetName(this);
		}

		inline Type* GetType() {

#if MONO
			RUNTIME_EXPORT_FUNC(ClassGetType, mono_class_get_type, Type*, Class*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(ClassGetType, il2cpp_class_get_type, Type*, Class*);
#endif

			return Export_ClassGetType(this);
		}

		template <typename T>
		inline BoxedValue<T>* Box(T* address) {

#if MONO
			RUNTIME_EXPORT_FUNC(Box, mono_value_box, BoxedValue<T>*, Domain*, Class*, void*);
			return Export_Box(Domain::GetRootDomain(), this, address);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(Box, il2cpp_value_box, BoxedValue<T>*, Class*, void*);
			return Export_Box(this, address);
#endif
		}

		inline bool IsSubclassOf(Class* parent) {

#if MONO
			RUNTIME_EXPORT_FUNC(IsSubclass, mono_class_is_subclass_of, bool, Class*, Class*, bool);
			return Export_IsSubclass(this, parent, true);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(IsSubclassOf, il2cpp_class_is_subclass_of, bool, Class*, Class*, bool);
			return Export_IsSubclass(this, parent, true);
#endif
		}

		inline Field* GetField(const char* name) {
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

		inline Method* GetMethod(const char* name, int index = 0) {
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

			uintptr_t iter{};
			int curIndex = index;
			while (Method* method = Export_GetMethod(this, &iter)) {
				if (strcmp(method->GetName(), name) == 0) {
					if (curIndex == 0) {
						_methods[this][name][index] = method;
						return method;
					}
					curIndex--;
				}
			}

			return nullptr;
		}

		template <typename T>
		inline void SetFieldValue(const char* name, T value, Object* instance = nullptr) {
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
		inline T GetFieldValue(const char* name, Object* instance = nullptr) {
			auto field = GetField(name);
			if (!field) {
				return T();
			}

			if (!instance)
				return field->Get<T>(GetVTable());
			else
				return field->Get<T>(instance);
		}
	};
}

namespace Types {

	class Object {
	public:
		static Runtime::Class* StaticRuntimeClass() {
			static Runtime::Class* _class = nullptr;
			if (_class) {
				return _class;
			}

#if MONO
			RUNTIME_EXPORT_FUNC(GetObjectClass, mono_get_object_class, Runtime::Class*);
			_class = Export_GetObjectClass();
#elif IL2CPP
			static Runtime::Class* obj = nullptr;
			if (!obj) {
				auto domain = Runtime::Domain::GetRootDomain();
				auto assembly = domain->GetAssembly("mscorlib");
				obj = assembly->GetClass("System", "Object");
			}
			_class = obj;
#endif
			return _class;
		}

		static Object* New() {
			auto klass = StaticRuntimeClass();
			if (!klass) {
				return nullptr;
			}

			return klass->New();
		}

		inline Runtime::Class* RuntimeClass() {
			auto vtable = Memory::Read<Runtime::VTable*>(THIS);
			if (!vtable) {
				return nullptr;
			}

			return vtable->GetClass();
		}

		inline Object* GetType() {
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

		inline bool IsInstanceOf(Runtime::Class* klass) {
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
		inline T* As() {
			if (!IsInstanceOf(T::StaticRuntimeClass())) {
				return nullptr;
			}

			return reinterpret_cast<T*>(this);
		}

		template <typename T>
		inline void SetFieldValue(const char* name, T value) {
			auto klass = RuntimeClass();
			if (!klass) {
				return;
			}

			klass->SetFieldValue(name, value, this);
		}

		template <typename T>
		inline T GetFieldValue(const char* name) {
			auto klass = RuntimeClass();
			if (!klass) {
				return T();
			}

			return klass->GetFieldValue<T>(name, this);
		}
	};

	template <typename T>
	class BoxedValue : public Object {
	public:
		inline T Unbox() {
#if MONO
			RUNTIME_EXPORT_FUNC(Unbox, mono_object_unbox, T, Object*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(Unbox, il2cpp_object_unbox, T, Object*);
#endif
			return Export_Unbox(this);
		}
	};

	class String : public Object {
	public:
		static Runtime::Class* StaticRuntimeClass() {
			static Runtime::Class* _class = nullptr;
			if (_class) {
				return _class;
			}

			static Runtime::Class* obj = nullptr;
			if (!obj) {
				auto domain = Runtime::Domain::GetRootDomain();
				auto assembly = domain->GetAssembly("mscorlib");
				obj = assembly->GetClass("System", "String");
			}
			_class = obj;
		}

		static String* New(const char* str) {
#if MONO
			RUNTIME_EXPORT_FUNC(StringNew, mono_string_new, String*, Runtime::Domain*, const char*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(StringNew, il2cpp_string_new, String*, const char*);
#endif

			return Export_StringNew(Runtime::Domain::GetRootDomain(), str);
		}

		inline const wchar_t* ToWString() {

#if MONO
			RUNTIME_EXPORT_FUNC(StringGetChars, mono_string_chars, const wchar_t*, String*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(StringGetChars, il2cpp_string_chars, const wchar_t*, String*);
#endif

			return Export_StringGetChars(this);
		}

		inline int GetLength() {

#if MONO
			RUNTIME_EXPORT_FUNC(StringGetLength, mono_string_length, int, String*);
#elif IL2CPP
			RUNTIME_EXPORT_FUNC(StringGetLength, il2cpp_string_length, int, String*);
#endif

			return Export_StringGetLength(this);
		}

		inline std::string ToString() {
			auto length = GetLength();
			auto chars = ToWString();
			std::string str;
			for (int i = 0; i < length; i++) {
				str += static_cast<char>(chars[i]);
			}
			return str;
		}

		inline bool operator==(const char* str) {
			return strcmp(ToString().c_str(), str) == 0;
		}

		inline bool operator==(const std::string& str) {
			return ToString() == str;
		}

		inline bool operator==(String* str) {
			return ToString() == str->ToString();
		}
	};
}