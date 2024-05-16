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

#define STRINGIFY(x) #x
#if MONO
#define RUNTIME_EXPORT_NAME(name) STRINGIFY(mono_##name)
#define RUNTIME_DLL "mono-2.0-bdwgc.dll"
#elif IL2CPP
#define RUNTIME_EXPORT_NAME(name) STRINGIFY(il2cpp_##name)
#define RUNTIME_DLL "GameAssembly.dll"
#endif

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

			uintptr_t address = (uintptr_t)GetProcAddress((HMODULE)_base, name.c_str());
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

		Module mod((uintptr_t)GetModuleHandleA(name.c_str()));
		_modules[name] = mod;
		return mod;
	}
}

namespace Runtime {
	// Forward declarations
	class Domain;


	// Implementations
	class Domain {
	private:

	public:
		
	};
}
