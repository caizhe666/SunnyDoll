#include "pch.h"
#include "ProxyCaller.h"
#include "detours/detours.h"

//
// File system lib
//

#include <filesystem>

namespace ProxyCaller {
	namespace Original {
		t_CreateProcessInternalW* pfnCreateProcessInternalW;
	}

	namespace Proxied {
		BOOL
			WINAPI
			CreateProcessInternalW(
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
			)
		{
			BOOL status;
			LPVOID DllPathBuffer;
			HMODULE hKernel32;
			std::string module_path;

			//
			// Make sure the app is running
			//

			status = Original::pfnCreateProcessInternalW(
				hToken,
				lpApplicationName,
				lpCommandLine,
				lpProcessAttributes,
				lpThreadAttributes,
				bInheritHandles,
				dwCreationFlags,
				lpEnvironment,
				lpCurrentDirectory,
				lpStartupInfo,
				lpProcessInformation,
				hNewToken
			);

			//
			// Inject our codes
			//

			if (!status) {
				return false;
			}

			hKernel32 = GetModuleHandleA("Kernel32.dll");

			if (!hKernel32) {
				return STATUS_INVALID_PARAMETER;
			}

			module_path = std::filesystem::current_path().string();

			DllPathBuffer = VirtualAllocEx(lpProcessInformation->hProcess, 0,
				module_path.size() + 1, MEM_COMMIT, PAGE_READWRITE);

			if (!DllPathBuffer) {
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			if (!WriteProcessMemory(lpProcessInformation->hProcess, DllPathBuffer,
				module_path.c_str(), module_path.size(), 0)) {
				VirtualFreeEx(lpProcessInformation->hProcess, DllPathBuffer, 0, MEM_RELEASE);
				return STATUS_INVALID_PARAMETER;
			}

			HANDLE hLoadThread = CreateRemoteThread(lpProcessInformation->hProcess, nullptr, 0,
				reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(
					hKernel32, "LoadLibraryA")
					), DllPathBuffer, 0, nullptr);

			if (!hLoadThread) {
				VirtualFreeEx(lpProcessInformation->hProcess, DllPathBuffer, 0, MEM_RELEASE);
				return STATUS_INVALID_PARAMETER;
			}

			WaitForSingleObject(hLoadThread, INFINITE);
			VirtualFreeEx(lpProcessInformation->hProcess, DllPathBuffer, 0, MEM_RELEASE);

			return status;
		}
	}

	std::unordered_map<PVOID, PVOID*> HookTable = {
		EMPLACE_PROXY(CreateProcessInternalW),
	};
}
