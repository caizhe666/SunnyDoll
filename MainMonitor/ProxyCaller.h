#pragma once

typedef
BOOL
(WINAPI
	t_CreateProcessInternalW)(
		HANDLE hToken,
		LPCWSTR lpApplicationName,
		LPWSTR lpCommandLine,
		LPSECURITY_ATTRIBUTES lpProcessAttributes,
		LPSECURITY_ATTRIBUTES lpThreadAttributes,
		BOOL bInheritHandles,
		DWORD dwCreationFlags,
		LPVOID lpEnvironment,
		LPCWSTR lpCurrentDirectory,
		LPSTARTUPINFOW lpStartupInfo,
		LPPROCESS_INFORMATION lpProcessInformation,
		PHANDLE hNewToken
		);

#define DECLEAR_ORIGINAL(Func) decltype(Func)* pfn##Func = Func
#define DECLEAR_EXTERNAL_ORIGINAL(Func) extern decltype(Func)* pfn##Func
#define DECLEAR_PROXIED(Func) decltype(Func) Func;
#define EMPLACE_PROXY(Func) { Proxied::Func,  reinterpret_cast<LPVOID*>(&Original::pfn##Func) }

namespace ProxyCaller {
	namespace Original {
		extern t_CreateProcessInternalW* pfnCreateProcessInternalW;
	}

	namespace Proxied {
		t_CreateProcessInternalW CreateProcessInternalW;
	}

	extern std::unordered_map<PVOID, PVOID*> HookTable;
}
