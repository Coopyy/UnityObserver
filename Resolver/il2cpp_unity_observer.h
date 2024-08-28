#pragma once
#ifndef UNITY_OBSERVER
#define UNITY_OBSERVER

#define DBG false

#include <cstdint>
#include <unordered_map>
#include <string>
#include <Windows.h>

#include <libs/Importer.h>

#pragma region Macros

#if DBG
#define DEBUGLOG(fmt, ...) Logger::DbgLog(fmt, __VA_ARGS__);
#else
#define DEBUGLOG(fmt, ...)
#endif

#ifdef _WIN64
#define CALLING_CONVENTION __fastcall
#elif _WIN32
#define CALLING_CONVENTION __cdecl
#endif

#define RUNTIME_DLL L"GameAssembly.dll"

#define THIS_PTR reinterpret_cast<uintptr_t>(this)
#define STRINGIFY(x) #x
#define RUNTIME_EXPORT_FUNC(name, export_name, type, ...)                \
    typedef type(CALLING_CONVENTION *t_##name)(__VA_ARGS__);             \
    static t_##name Export_##name = nullptr;                             \
    if (!Export_##name)                                                  \
    {                                                                    \
        auto mod = Memory::GetModule(RUNTIME_DLL);                       \
        Export_##name = mod.GetExport<t_##name>(STRINGIFY(export_name)); \
    }

#define ASSERT_THIS()                             \
    if (!this)                                    \
    {                                             \
        Logger::LogError("Invalid THIS pointer"); \
        return {};                                \
    }

#define DEFINE_STATIC_RUNTIME_CLASS(assembly, nmspace, name)                                          \
    static Class *StaticRuntimeClass()                                                       \
    {                                                                                                 \
        static Class *klass = nullptr;                                                       \
        if (!klass)                                                                                   \
        {                                                                                             \
            klass = Domain::GetRootDomain()->GetAssembly(assembly)->GetClass(nmspace, name); \
        }                                                                                             \
        return klass;                                                                                 \
    } \
    static Object *New() \
    { \
        return StaticRuntimeClass()->New(); \
    }
#pragma endregion

namespace UO
{
#pragma region Class Forward Decls
    class Domain;
    class Assembly;
    class Image;
    class Class;
    class Method;
    class Field;
    class Type;
    class VTable;

    typedef enum Il2CppTypeEnum : byte
    {
        IL2CPP_TYPE_END = 0x00, /* End of List */
        IL2CPP_TYPE_VOID = 0x01,
        IL2CPP_TYPE_BOOLEAN = 0x02,
        IL2CPP_TYPE_CHAR = 0x03,
        IL2CPP_TYPE_I1 = 0x04,
        IL2CPP_TYPE_U1 = 0x05,
        IL2CPP_TYPE_I2 = 0x06,
        IL2CPP_TYPE_U2 = 0x07,
        IL2CPP_TYPE_I4 = 0x08,
        IL2CPP_TYPE_U4 = 0x09,
        IL2CPP_TYPE_I8 = 0x0a,
        IL2CPP_TYPE_U8 = 0x0b,
        IL2CPP_TYPE_R4 = 0x0c,
        IL2CPP_TYPE_R8 = 0x0d, // last primitive type value

        IL2CPP_TYPE_STRING = 0x0e,
        IL2CPP_TYPE_PTR = 0x0f,         /* arg: <type> token */
        IL2CPP_TYPE_BYREF = 0x10,       /* arg: <type> token */
        IL2CPP_TYPE_VALUETYPE = 0x11,   /* arg: <type> token */
        IL2CPP_TYPE_CLASS = 0x12,       /* arg: <type> token */
        IL2CPP_TYPE_VAR = 0x13,         /* Generic parameter in a generic type definition, represented as number (compressed unsigned integer) number */
        IL2CPP_TYPE_ARRAY = 0x14,       /* type, rank, boundsCount, bound1, loCount, lo1 */
        IL2CPP_TYPE_GENERICINST = 0x15, /* <type> <type-arg-count> <type-1> \x{2026} <type-n> */
        IL2CPP_TYPE_TYPEDBYREF = 0x16,
        IL2CPP_TYPE_I = 0x18,
        IL2CPP_TYPE_U = 0x19,
        IL2CPP_TYPE_FNPTR = 0x1b, /* arg: full method signature */
        IL2CPP_TYPE_OBJECT = 0x1c,
        IL2CPP_TYPE_SZARRAY = 0x1d,   /* 0-based one-dim-array */
        IL2CPP_TYPE_MVAR = 0x1e,      /* Generic parameter in a generic method definition, represented as number (compressed unsigned integer)  */
        IL2CPP_TYPE_CMOD_REQD = 0x1f, /* arg: typedef or typeref token */
        IL2CPP_TYPE_CMOD_OPT = 0x20,  /* optional arg: typedef or typref token */
        IL2CPP_TYPE_INTERNAL = 0x21,  /* CLR internal type */

        IL2CPP_TYPE_MODIFIER = 0x40, /* Or with the following types */
        IL2CPP_TYPE_SENTINEL = 0x41, /* Sentinel for varargs method signature */
        IL2CPP_TYPE_PINNED = 0x45,   /* Local var that points to pinned object */

        IL2CPP_TYPE_ENUM = 0x55 /* an enumeration */
    } Il2CppTypeEnum;

    class Object;

    template <typename T>
    class BoxedValue;
#pragma endregion

#pragma region Decls
    class Logger
    {
    public:
        static void Setup();
        static void Cleanup();
        static void Log(const char* fmt, ...);
        static void LogError(const char* fmt, ...);
        static void LogException(Object* exception);
        static void DbgLog(const char* fmt, ...);
    };

    class Memory
    {
    public:
        class Module
        {
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
        static T Read(uintptr_t address);

        template <typename T>
        static void Write(uintptr_t address, T value);

        static Module GetModule(std::wstring name);
    };

    class Method
    {
        void* GetThunk();

    public:
        const char* GetName();
        unsigned int GetToken();
        Class* GetClass();
        int GetParameterCount();

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
         * @brief Invokes the method using the compiled function pointer directly.
         * This is the fastest method of invocation, but has no type safety or exception handling.
         * Only use this if you are absolutely sure the method signature matches the template arguments.
         *
         * @tparam T The return type of the method. Defaults to void if not specified.
         * @param instance The instance on which to invoke the method. If null, the method will be invoked as a static method.
         * @param args The arguments to pass to the method. Primitives and enums are passed by value. Value types are passed as pointers to their value.
         * Reference and string types should be passed as Object pointers.
         *
         * @return The return value of the method. Value return types will NOT be boxed.
         */
        template <typename T = void, typename I, typename... Args>
        T InvokeUnsafe(I instance, Args... args);

        /**
         * @brief Retrieves the thunk for the method with specified signature.
         *
         * @tparam T The return type of the method.
         * @tparam Args The argument types of the method.
         * @return A function pointer with the specified signature representing the method thunk.
         * The first parameter, if not static, is the instance on which to invoke the method.
         * The last argument is a pointer to an Object pointer which will be set to the exception if one is thrown, null to not catch exceptions.
         */
        template <typename T, typename... Args>
        T(CALLING_CONVENTION* GetThunk())
            (Args..., Object**);

        void* GetCompiled();
        /**
         * @brief Retrieves the compiled function pointer for the method with specified signature.
         *
         * @tparam T The return type of the method.
         * @tparam Args The argument types of the method.
         * @return A function pointer with the specified signature representing the method thunk.
         * The first parameter, if not static, is the instance on which to invoke the method.
         */
        template <typename T, typename... Args>
        T(CALLING_CONVENTION* GetCompiled())
            (Args...);
    };

    class Field
    {
    public:
        const char* GetName();
        uintptr_t GetOffset();

        template <typename T>
        T Get(Object* instance);

        template <typename T>
        void Set(Object* instance, T& value);

        template <typename T>
        T Get();

        template <typename T>
        void Set(T value);
    };

    class Assembly
    {
    public:
        Image* GetImage();
        const char* GetName();
        Class* GetClass(const char* nameSpace, const char* name);
    };

    class Domain
    {
    public:
        static Domain* GetRootDomain();
        void ThreadAttach();
        std::vector<Assembly*> GetAssemblies();
        Assembly* GetAssembly(const char* name);
    };

    class Type
    {
    public:
        Object* GetSystemType();
        Il2CppTypeEnum GetTypeEnum();
    };

    class Class
    {
        bool GetBitFieldValue(int index);
    public:
        static Class* Find(const char* fullName);

        Object* New();
        const char* GetName();
        Type* GetType();
        Class* GetParent();
        bool IsSubclassOf(Class* parent);
        bool IsValueType();
        Field* GetField(const char* name);
        Method* GetMethod(const char* name, int index = 0, bool checkParents = true);

        std::vector<Field*> GetFields();
        std::vector<Method*> GetMethods();

        /**
        * @brief Boxes the specified value type.
        *
        * @tparam T The type of the value to box.
        * @param address The address of the value to box.
        *
        * @return A boxed value object containing the value.
        */
        template <typename T>
        BoxedValue<T>* Box(T* address);

        /**
         * @brief Invokes the method with the specified name and arguments.
         *
         * @param name The name of the method to invoke.
         * @param instance The instance on which to invoke the method. If null, the method will be invoked as a static method.
         * @param args The arguments to pass to the method. Value and enum types should be passed as pointers to the value.
         *
         * @return The return value of the method. If the return type is a value type, it will be boxed.
         */
        template <typename T = void, typename I, typename... Args>
        T InvokeMethod(const char* name, I instance, Args... args);

        template <typename T>
        void SetFieldValue(const char* name, T value, Object* instance = nullptr);

        template <typename T>
        T GetFieldValue(const char* name, Object* instance = nullptr);
    };

    class Object;
    class String;

    class Object
    {
    public:
        static Class* StaticRuntimeClass();
        static Object* New();
        Class* RuntimeClass();
        Object* GetType();
        bool IsInstanceOf(Class* klass);
        String* ToString();
        bool IsNull();

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
    class BoxedValue : public Object
    {
        void* UnboxToPtr();

    public:
        T Unbox();
        String* ToString();

        template <typename T = void, typename... Args>
        T InvokeMethod(const char* name, Args... args);

        void Set(T value);
    };

    class String : public Object
    {
    public:
        static Class* StaticRuntimeClass();
        static String* New(const char* str);
        const wchar_t* ToWide();
        int Length();
        std::string ToCPP();
        bool operator==(const char* str);
        bool operator==(const std::string& str);
        bool operator==(String* str);
    };

    template <typename T>
    class GCHandle
    {
        T* _object;
        void* _handle;

    public:
        GCHandle(T* object, bool pinned = false);
        ~GCHandle();
        T* Get();
    };
#pragma endregion

#pragma region Defs
#pragma region Utils

    inline void Logger::Setup()
    {
        AllocConsole();
        freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(0)), "conin$", "r", static_cast<_iobuf*>(__acrt_iob_func(0)));
        freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(1)), "conout$", "w", static_cast<_iobuf*>(__acrt_iob_func(1)));
        freopen_s(reinterpret_cast<_iobuf**>(__acrt_iob_func(2)), "conout$", "w", static_cast<_iobuf*>(__acrt_iob_func(2)));
    }

    inline void Logger::Cleanup()
    {
        fclose(static_cast<_iobuf*>(__acrt_iob_func(0)));
        fclose(static_cast<_iobuf*>(__acrt_iob_func(1)));
        fclose(static_cast<_iobuf*>(__acrt_iob_func(2)));
        FreeConsole();
    }

    inline void Logger::Log(const char* fmt, ...)
    {
        printf("[*] ");
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }

    inline void Logger::LogError(const char* fmt, ...)
    {
        printf("[!] ");
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }
    inline void Logger::LogException(Object* exception)
    {
        auto message = exception->InvokeMethod<String*>("ToString");
        if (message)
            Logger::LogError("Exception: %s", message->ToCPP().c_str());
    }
    inline void Logger::DbgLog(const char* fmt, ...)
    {
        printf("[DBG] ");
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");

        static FILE* logFile = fopen("log.txt", "a");
        if (logFile)
        {
            fprintf(logFile, "[DBG] ");
            va_list args;
            va_start(args, fmt);
            vfprintf(logFile, fmt, args);
            va_end(args);
            fprintf(logFile, "\n");
        }
        else
        {
            printf("[!] Failed to open log file for writing.\n");
        }
    }
    Memory::Module::Module(uintptr_t base) : _base(base) {}
    Memory::Module::Module() : _base(0) {}

    inline uintptr_t Memory::Module::GetBase()
    {
        return _base;
    }

    template <typename T>
    inline T Memory::Module::GetExport(std::string name)
    {
        if (_base == 0)
        {
            return nullptr;
        }

        if (_exports.find(name) != _exports.end())
        {
            return reinterpret_cast<T>(_exports[name]);
        }

        uintptr_t address = (uintptr_t)Importer::GetExport(_base, name.c_str()); // Note: Can change to getting export with export table
        if (address == 0)
        {
            Logger::LogError("Failed to get export: %s", name.c_str());
            return nullptr;
        }

        _exports[name] = address;
        return reinterpret_cast<T>(address);
    }

    template <typename T>
    inline T Memory::Read(uintptr_t address)
    {
        return *reinterpret_cast<T*>(address);
    }

    template <typename T>
    inline void Memory::Write(uintptr_t address, T value)
    {
        *reinterpret_cast<T*>(address) = value;
    }

    inline Memory::Module Memory::GetModule(std::wstring name)
    {
        static std::unordered_map<std::wstring, Memory::Module> _modules;
        if (_modules.find(name) != _modules.end())
        {
            return _modules[name];
        }

        Memory::Module mod(Importer::GetModuleBase(name.c_str()));
        if (mod.GetBase() == 0)
        {
            Logger::LogError("Failed to get module: %s", name.c_str());
            return {};
        }

        _modules[name] = mod;
        return mod;
    }
#pragma endregion

    // Runtime API
#pragma region Method
    inline const char* Method::GetName()
    {
        RUNTIME_EXPORT_FUNC(MethodGetName, il2cpp_method_get_name, const char*, Method*);
        return Export_MethodGetName(this);
    }

    inline int Method::GetParameterCount()
    {
        RUNTIME_EXPORT_FUNC(MethodGetParameterCount, il2cpp_method_get_param_count, int, Method*);
        return Export_MethodGetParameterCount(this);
    }

    template <typename T, typename... Args>
    inline T Method::Invoke(Object* instance, Args... args)
    {
        DEBUGLOG("Invoking method: %s", GetName());
        RUNTIME_EXPORT_FUNC(MethodInvoke, il2cpp_runtime_invoke, Object*, Method*, Object*, void**, Object**);
        Object* exception = nullptr;
        Object* result;
        if constexpr (sizeof...(Args) > 0)
        {
            void* params[] = { args... };
            result = Export_MethodInvoke(this, instance, params, &exception);
        }
        else
            result = Export_MethodInvoke(this, instance, nullptr, &exception);

        if (exception)
            Logger::LogException(exception);

        DEBUGLOG("Method invoked: %s. Returned %p", GetName(), result);

        if constexpr (!std::is_same_v<T, void>)
            return reinterpret_cast<T>(result);
    }

    template <typename T, typename I, typename... Args>
    inline T Method::InvokeUnsafe(I instance, Args... args)
    {
        auto thunk = GetCompiled();
        if (!thunk)
        {
            if constexpr (std::is_same_v<T, void>)
                return;
            return T();
        }

        auto invoke = [&]<typename... IArgs>(IArgs... invokeArgs)
        {
            return reinterpret_cast<T(CALLING_CONVENTION*)(IArgs...)>(thunk)(invokeArgs...);
        };

        if constexpr (std::is_pointer_v<I> || std::is_same_v<I, std::nullptr_t>)
            return instance ? invoke(instance, args...) : invoke(args...);
        else
            return invoke(instance, args...);
    }

    inline void* Method::GetThunk()
    {
        static std::unordered_map<Method*, void*> _thunks;
        if (_thunks.find(this) != _thunks.end())
        {
            return _thunks[this];
        }

        RUNTIME_EXPORT_FUNC(MethodGetThunk, il2cpp_method_get_unmanaged_thunk, void*, Method*);

        auto thunk = Export_MethodGetThunk(this);
        if (!thunk)
        {
            Logger::LogError("Failed to get method thunk: %s", GetName());
            return nullptr;
        }

        _thunks[this] = thunk;
        return thunk;
    }

    inline void* Method::GetCompiled()
    {
        auto ptr = Memory::Read<void*>(THIS_PTR);

        if (!ptr)
        {
            Logger::LogError("Failed to compile method: %s", GetName());
            return nullptr;
        }

        return ptr;
    }

    template <typename T, typename... Args>
    inline T(CALLING_CONVENTION* Method::GetThunk())(Args..., Object**)
    {
        return reinterpret_cast<T(CALLING_CONVENTION*)(Args..., Object**)>(GetThunk());
    }

    template <typename T, typename... Args>
    inline T(CALLING_CONVENTION* Method::GetCompiled())(Args...)
    {
        return reinterpret_cast<T(CALLING_CONVENTION*)(Args...)>(GetCompiled());
    }

    inline unsigned int Method::GetToken()
    {
        RUNTIME_EXPORT_FUNC(MethodGetToken, il2cpp_method_get_token, unsigned int, Method*);
        return Export_MethodGetToken(this);
    }

    inline Class* Method::GetClass()
    {
        RUNTIME_EXPORT_FUNC(MethodGetClass, il2cpp_method_get_class, Class*, Method*);
        return Export_MethodGetClass(this);
    }
#pragma endregion
#pragma region Field
    inline const char* Field::GetName()
    {
        RUNTIME_EXPORT_FUNC(FieldGetName, il2cpp_field_get_name, const char*, Field*);
        return Export_FieldGetName(this);
    }

    inline uintptr_t Field::GetOffset()
    {
        static std::unordered_map<Field*, uintptr_t> _offsets;
        if (_offsets.find(this) != _offsets.end())
        {
            return _offsets[this];
        }

        RUNTIME_EXPORT_FUNC(FieldGetOffset, il2cpp_field_get_offset, uintptr_t, Field*);
        auto offset = Export_FieldGetOffset(this);
        _offsets[this] = offset;
        return offset;
    }

    template <typename T>
    inline T Field::Get(Object* instance)
    {
        DEBUGLOG("Getting field: %s", GetName());
        T value;

        RUNTIME_EXPORT_FUNC(FieldGetValue, il2cpp_field_get_value, void, Object*, Field*, void*);
        Export_FieldGetValue(instance, this, &value);
        DEBUGLOG("Field got: %s. Value: %p", GetName(), value);
        return value;
    }

    template <typename T>
    inline void Field::Set(Object* instance, T& value)
    {
        DEBUGLOG("Setting field: %s", GetName());
#if MONO
        RUNTIME_EXPORT_FUNC(FieldSetValue, mono_field_set_value, void, Object*, Field*, void*);
#elif IL2CPP
        RUNTIME_EXPORT_FUNC(FieldSetValue, il2cpp_field_set_value, void, Object*, Field*, void*);
#endif
        Export_FieldSetValue(instance, this, &value);
        DEBUGLOG("Field set: %s. Value: %p", GetName(), value);
    }

    template <typename T>
    inline T Field::Get()
    {
        DEBUGLOG("Getting static field: %s", GetName());
        T value;

        RUNTIME_EXPORT_FUNC(FieldGetStaticValue, il2cpp_field_static_get_value, void, Field*, void*);
        Export_FieldGetStaticValue(this, &value);
        DEBUGLOG("Static field got: %s. Value: %p", GetName(), value);
        return value;
    }

    template <typename T>
    inline void Field::Set(T value)
    {
        DEBUGLOG("Setting static field: %s", GetName());
        RUNTIME_EXPORT_FUNC(FieldSetStaticValue, il2cpp_field_static_set_value, void, Field*, void*);

        Export_FieldSetStaticValue(this, &value);
        DEBUGLOG("Static field set: %s. Value: %p", GetName(), value);
    }
#pragma endregion
#pragma region Assembly
    inline Image* Assembly::GetImage()
    {
        RUNTIME_EXPORT_FUNC(AssemblyGetImage, il2cpp_assembly_get_image, Image*, Assembly*);
        return Export_AssemblyGetImage(this);
    }
    inline const char* Assembly::GetName()
    {
        RUNTIME_EXPORT_FUNC(ImageGetName, il2cpp_image_get_name, const char*, Image*);
        return Export_ImageGetName(GetImage());
    }

    inline Class* Assembly::GetClass(const char* nameSpace, const char* name)
    {
        auto image = GetImage();
        if (!image)
        {
            return nullptr;
        }

        RUNTIME_EXPORT_FUNC(ImageGetClass, il2cpp_class_from_name, Class*, Image*, const char*, const char*);
        return Export_ImageGetClass(image, nameSpace, name);
    }
#pragma endregion
#pragma region Domain
    inline Domain* Domain::GetRootDomain()
    {
        RUNTIME_EXPORT_FUNC(GetDomain, il2cpp_domain_get, Domain*);

        static Domain* rootDomain = nullptr;
        if (!rootDomain)
        {

            rootDomain = Export_GetDomain();
            rootDomain->ThreadAttach();
            Logger::Log("UnityObserver initialized with Root Domain: %p", rootDomain);
        }

        return rootDomain;
    }

    inline void Domain::ThreadAttach()
    {
        RUNTIME_EXPORT_FUNC(ThreadAttach, il2cpp_thread_attach, void, Domain*);
        Export_ThreadAttach(this);
    }

    inline std::vector<Assembly*> Domain::GetAssemblies()
    {
        std::vector<Assembly*> assemblies;

        RUNTIME_EXPORT_FUNC(DomainGetAssemblies, il2cpp_domain_get_assemblies, Assembly**, Domain*, size_t*);

        size_t size;
        auto _assemblies = Export_DomainGetAssemblies(this, &size);
        for (size_t i = 0; i < size; i++)
            assemblies.push_back(_assemblies[i]);

        return assemblies;
    }

    inline Assembly* Domain::GetAssembly(const char* name)
    {

        static std::unordered_map<std::string, Assembly*> _assemblies;
        if (_assemblies.find(name) != _assemblies.end())
        {
            return _assemblies[name];
        }

        for (const auto assembly : GetAssemblies())
        {
            auto assemblyName = assembly->GetName();
            _assemblies[assemblyName] = assembly;
            if (strcmp(assemblyName, name) == 0)
            {
                return assembly;
            }
        }

        Logger::LogError("Failed to get assembly: '%s'", name);
        return nullptr;
    }
#pragma endregion
#pragma region Type
    inline Object* Type::GetSystemType()
    {
        RUNTIME_EXPORT_FUNC(TypeGetSystemType, il2cpp_type_get_object, Object*, Type*);
        return Export_TypeGetSystemType(this);
    }

    inline Il2CppTypeEnum Type::GetTypeEnum()
    {
        return Memory::Read<Il2CppTypeEnum>(THIS_PTR + 0x10);
    }
#pragma endregion
#pragma region Class
    inline Class* Class::Find(const char* fullName)
    {
        static std::unordered_map<std::string, Class*> _classes;

        if (_classes.find(fullName) != _classes.end())
        {
            return _classes[fullName];
        }

        std::string name(fullName);
        std::string nameSpace;
        std::string className = name;

        size_t lastDot = name.find_last_of('.');
        if (lastDot != std::string::npos)
        {
            nameSpace = name.substr(0, lastDot);
            className = name.substr(lastDot + 1);
        }

        auto domain = Domain::GetRootDomain();
        for (auto assembly : domain->GetAssemblies())
        {
            auto klass = assembly->GetClass(nameSpace.empty() ? "" : nameSpace.c_str(), className.c_str());
            if (klass)
            {
                _classes[fullName] = klass;
                return klass;
            }
        }

        return nullptr;
    }

    inline bool Class::GetBitFieldValue(int index)
    {
        return (Memory::Read<unsigned char>(THIS_PTR + 0x18) >> index) & 1;
    }

    inline Object* Class::New()
    {
        RUNTIME_EXPORT_FUNC(ObjectNew, il2cpp_object_new, Object*, Domain*, Class*);
        return Export_ObjectNew(Domain::GetRootDomain(), this);
    }

    inline const char* Class::GetName()
    {
        RUNTIME_EXPORT_FUNC(ClassGetName, il2cpp_class_get_name, const char*, Class*);
        return Export_ClassGetName(this);
    }

    inline Type* Class::GetType()
    {
        RUNTIME_EXPORT_FUNC(ClassGetType, il2cpp_class_get_type, Type*, Class*);
        return Export_ClassGetType(this);
    }

    inline Class* Class::GetParent()
    {
        RUNTIME_EXPORT_FUNC(ClassGetParent, il2cpp_class_get_parent, Class*, Class*);
        return Export_ClassGetParent(this);
    }

    template <typename T>
    inline BoxedValue<T>* Class::Box(T* address)
    {
        RUNTIME_EXPORT_FUNC(Box, il2cpp_value_box, BoxedValue<T> *, Class*, void*);
        return Export_Box(this, address);
    }

    inline bool Class::IsSubclassOf(Class* parent)
    {
        RUNTIME_EXPORT_FUNC(IsSubclass, il2cpp_class_is_subclass_of, bool, Class*, Class*, bool);
        return Export_IsSubclass(this, parent, true);
    }

    inline bool Class::IsValueType()
    {
        return GetBitFieldValue(1) || GetBitFieldValue(2) || GetType()->GetTypeEnum() == IL2CPP_TYPE_I; // idk why boolean showing as I??
    }

    inline Field* Class::GetField(const char* name)
    {
        static std::unordered_map<Class*, std::unordered_map<std::string, Field*>> _fields;
        if (_fields.find(this) != _fields.end())
        {
            auto& fields = _fields[this];
            if (fields.find(name) != fields.end())
            {
                return fields[name];
            }
        }

        RUNTIME_EXPORT_FUNC(GetField, il2cpp_class_get_field_from_name, Field*, Class*, const char*);

        Field* field = Export_GetField(this, name);
        if (field)
            _fields[this][name] = field;
        else
            Logger::LogError("Failed to get field: '%s'", name);

        return field;
    }

    inline Method* Class::GetMethod(const char* name, int index, bool checkParents)
    {
        static std::unordered_map<Class*, std::unordered_map<std::string, std::unordered_map<int, Method*>>> _methods;

        if (_methods.find(this) != _methods.end())
        {
            auto& methods = _methods[this];
            if (methods.find(name) != methods.end())
            {
                auto& overloads = methods[name];
                if (overloads.find(index) != overloads.end())
                {
                    return overloads[index];
                }
            }
        }

        RUNTIME_EXPORT_FUNC(GetMethod, il2cpp_class_get_methods, Method*, Class*, uintptr_t*);

        Class* klass = this;
        while (klass)
        {
            uintptr_t iter{};
            int curIndex = index;
            while (Method* method = Export_GetMethod(klass, &iter))
            {
                if (strcmp(method->GetName(), name) == 0)
                {
                    if (curIndex == 0)
                    {
                        _methods[klass][name][index] = method;
                        return method;
                    }
                    curIndex--;
                }
            }

            if (!checkParents)
            {
                break;
            }

            klass = klass->GetParent();
        }

        Logger::LogError("Failed to get method: '%s'", name);
        return nullptr;
    }

    inline std::vector<Field*> Class::GetFields()
    {
        std::vector<Field*> fields;

        RUNTIME_EXPORT_FUNC(GetFields, il2cpp_class_get_fields, Field*, Class*, uintptr_t*);

        uintptr_t iter{};
        while (Field* field = Export_GetFields(this, &iter))
            fields.push_back(field);

        return fields;
    }

    inline std::vector<Method*> Class::GetMethods()
    {
        std::vector<Method*> methods;

        RUNTIME_EXPORT_FUNC(GetMethods, il2cpp_class_get_methods, Method*, Class*, uintptr_t*);

        uintptr_t iter{};
        while (Method* method = Export_GetMethods(this, &iter))
            methods.push_back(method);

        return methods;
    }

    template <typename T, typename I, typename... Args>
    inline T Class::InvokeMethod(const char* name, I instance, Args... args)
    {
        static_assert(std::is_void_v<T> || std::is_pointer_v<T>, "Return type must be a pointer type. if youre expecting a value type, its boxed.");

        auto method = GetMethod(name);
        if (method)
        {
            return method->DEFAULT_INVOKATION<T>(instance, args...);
        }
        Logger::LogError("Failed to invoke method '%s' from class '%s'", name, GetName());
        return T();
    }

    template <typename T>
    inline void Class::SetFieldValue(const char* name, T value, Object* instance)
    {
        auto field = GetField(name);
        if (!field)
        {
            return;
        }

        if (!instance)
            field->Set(value);
        else
            field->Set(instance, value);
    }

    template <typename T>
    inline T Class::GetFieldValue(const char* name, Object* instance)
    {
        auto field = GetField(name);
        if (!field)
        {
            return T();
        }

        if (!instance)
            return field->Get<T>();
        else
            return field->Get<T>(instance);
    }
#pragma endregion

    // Types
#pragma region Object
    inline Class* Object::StaticRuntimeClass()
    {
        static Class* _class = nullptr;
        if (_class)
        {
            return _class;
        }

        auto domain = Domain::GetRootDomain();
        auto assembly = domain->GetAssembly("mscorlib");
        _class = assembly->GetClass("System", "Object");

        return _class;
    }

    inline Object* Object::New()
    {
        auto klass = StaticRuntimeClass();
        if (!klass)
        {
            return nullptr;
        }

        return klass->New();
    }

    inline Class* Object::RuntimeClass()
    {
        ASSERT_THIS();
        return Memory::Read<Class*>(THIS_PTR);
    }

    inline Object* Object::GetType()
    {
        ASSERT_THIS();
        auto klass = RuntimeClass();
        if (!klass)
        {
            return nullptr;
        }

        auto type = klass->GetType();
        if (!type)
        {
            return nullptr;
        }

        return type->GetSystemType();
    }

    inline bool Object::IsInstanceOf(Class* klass)
    {
        ASSERT_THIS();
        auto curClass = RuntimeClass();
        if (!curClass)
        {
            return false;
        }

        if (curClass == klass)
        {
            return true;
        }

        return curClass->IsSubclassOf(klass);
    }

    template <typename T>
    inline T* Object::As()
    {
        ASSERT_THIS();
        if (!IsInstanceOf(T::StaticRuntimeClass()))
        {
            return nullptr;
        }

        return reinterpret_cast<T*>(this);
    }

    template <typename T>
    inline void Object::SetFieldValue(const char* name, T value)
    {
        if (!this)
        {
            Logger::LogError("Tried to set field on null object");
            return;
        }
        auto klass = RuntimeClass();
        if (!klass)
        {
            return;
        }

        klass->SetFieldValue(name, value, this);
    }

    template <typename T>
    inline T Object::GetFieldValue(const char* name)
    {
        ASSERT_THIS();
        auto klass = RuntimeClass();
        if (!klass)
        {
            Logger::LogError("Runtime class was null when getting field: %s", name);
            return T();
        }

        return klass->GetFieldValue<T>(name, this);
    }

    template <typename T, typename... Args>
    inline T Object::InvokeMethod(const char* name, Args... args)
    {
        ASSERT_THIS();
        return RuntimeClass()->InvokeMethod<T>(name, this, args...);
    }

    inline String* Object::ToString()
    {
        return InvokeMethod<String*>("ToString");
    }

    // this is for UnityEngine.Object
    inline bool Object::IsNull()
    { // m_cachedPtr == nullptr
        return !this || Memory::Read<uintptr_t>(THIS_PTR + 0x10) == 0;
    }
#pragma endregion
#pragma region BoxedValue
    template <typename T>
    inline void* BoxedValue<T>::UnboxToPtr()
    {
        if (!this)
        {
            Logger::LogError("Tried to unbox null object");
            return nullptr;
        }

        if (!RuntimeClass()->IsValueType())
        {
            Logger::LogError("Tried to unbox non-value type: %s %p", RuntimeClass()->GetName(), RuntimeClass()->GetType()->GetTypeEnum());
            return nullptr;
        }
        RUNTIME_EXPORT_FUNC(Unbox, il2cpp_object_unbox, void*, Object*);
        return Export_Unbox(this);
    }

    template <typename T>
    inline T BoxedValue<T>::Unbox()
    {
        auto unboxed = UnboxToPtr();
        if (!unboxed)
            return T();

        return Memory::Read<T>(reinterpret_cast<uintptr_t>(unboxed));
    }

    template <typename C>
    template <typename T, typename... Args>
    inline T BoxedValue<C>::InvokeMethod(const char* name, Args... args)
    {
        auto unboxed = UnboxToPtr();
        if (!unboxed)
        {
            return T();
        }

        return RuntimeClass()->InvokeMethod<T>(name, reinterpret_cast<Object*>(unboxed), args...);
    }

    template <typename T>
    inline void BoxedValue<T>::Set(T value)
    {
        auto unboxed = UnboxToPtr();
        if (!unboxed)
            return;

        Memory::Write(reinterpret_cast<uintptr_t>(unboxed), value);
    }

    template <typename T>
    inline String* BoxedValue<T>::ToString()
    {
        return InvokeMethod<String*>("ToString");
    }

#pragma endregion
#pragma region String
    inline Class* String::StaticRuntimeClass()
    {
        static Class* _class = nullptr;
        if (_class)
            return _class;

        auto domain = Domain::GetRootDomain();
        auto assembly = domain->GetAssembly("mscorlib");
        _class = assembly->GetClass("System", "String");

        return _class;
    }

    inline String* String::New(const char* str)
    {
        RUNTIME_EXPORT_FUNC(StringNew, il2cpp_string_new_wrapper, String*, const char*);
        return Export_StringNew(str);
    }

    inline const wchar_t* String::ToWide()
    {
        RUNTIME_EXPORT_FUNC(StringGetChars, il2cpp_string_chars, const wchar_t*, String*);
        return Export_StringGetChars(this);
    }

    inline int String::Length()
    {
        RUNTIME_EXPORT_FUNC(StringGetLength, il2cpp_string_length, int, String*);
        return Export_StringGetLength(this);
    }

    inline std::string String::ToCPP()
    {
        auto length = Length();
        auto chars = ToWide();
        std::string str;
        for (int i = 0; i < length; i++)
        {
            str += static_cast<char>(chars[i]);
        }
        return str;
    }

    inline bool String::operator==(const char* str)
    {
        return strcmp(ToCPP().c_str(), str) == 0;
    }

    inline bool String::operator==(const std::string& str)
    {
        return ToCPP() == str;
    }

    inline bool String::operator==(String* str)
    {
        return ToCPP() == str->ToCPP();
    }
#pragma endregion
#pragma region CGHandle
    template <typename T>
    inline GCHandle<T>::GCHandle(T* object, bool pinned) : _object(object)
    {
        RUNTIME_EXPORT_FUNC(GCHandleNew, il2cpp_gchandle_new, void*, T*, bool);
        _handle = Export_GCHandleNew(object, pinned);
    }

    template <typename T>
    inline GCHandle<T>::~GCHandle()
    {
        RUNTIME_EXPORT_FUNC(GCHandleFree, il2cpp_gchandle_free, void, void*);
        Export_GCHandleFree(_handle);
    }

    template <typename T>
    inline T* GCHandle<T>::Get()
    {
        return _object;
    }
#pragma endregion

#pragma endregion

} // namespace UnityObserver
#endif
