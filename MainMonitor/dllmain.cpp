// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "detours/detours.h"
#include "ProxyCaller.h"

#include "pugixml.hpp"

HMODULE g_hModule = nullptr;
HMODULE g_hNtdll = nullptr;
HMODULE g_hKernelBase = nullptr;

BOOL
OnProcessAttach(
	VOID
)
{
	//
	// Get dll handles
	//

	g_hNtdll = GetModuleHandle(L"ntdll.dll");
	g_hKernelBase = GetModuleHandleA("KernelBase.dll");

	if (!g_hNtdll || !g_hKernelBase) {
		return false;
	}

	ProxyCaller::Original::pfnCreateProcessInternalW =
		reinterpret_cast<t_CreateProcessInternalW*>(
			GetProcAddress(g_hKernelBase, "CreateProcessInternalW"));

	if (!ProxyCaller::Original::pfnCreateProcessInternalW) {
		return false;
	}

	//
	// Install hooks
	//

	for (auto& Item : ProxyCaller::HookTable) {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(Item.second, Item.first);
		DetourTransactionCommit();
	}

	return true;
}

VOID
OnProcessDetach(
	VOID
)
{
	//
	// Restore every hooks
	//

	for (auto& Item : ProxyCaller::HookTable) {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(Item.second, Item.first);
		DetourTransactionCommit();
	}
}

BOOL
APIENTRY
DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (DetourIsHelperProcess()) {
		return true;
	}

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//
		// Save module handle
		//

		g_hModule = hModule;

		DetourRestoreAfterWith();

		OnProcessAttach();

		break;
	case DLL_PROCESS_DETACH:
		OnProcessDetach();

		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return true;
}
