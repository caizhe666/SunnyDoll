#include "stdafx.h"

//
// Argument parser lib
//

#undef max
#include "argparse.hpp"

//
// Hook lib
//

#include "detours/detours.h"

//
// Config lib
//

#include "ConfigProvider.h"

//
// Program main entry
//

#if defined(_M_X64)
#define MMONITOR_NAME "\\MainMonitor_x64.dll"
#endif // x64

#if defined(_M_IX86)
#define MMONITOR_NAME "\\MainMonitor_x86.dll"
#endif // x86

INT
wmain(
	INT argc,
	PWSTR argv[],
	PWSTR envp[]
)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	spdlog::set_level(spdlog::level::trace);
#else // DEBUG
	spdlog::set_level(spdlog::level::info);
#endif // !DEBUG

	spdlog::info("SunnyDoll starting... ");

	//
	// UTF-8 Unicode converter
	//

	using convert_type = std::codecvt_utf8<WCHAR>;
	std::wstring_convert<convert_type, WCHAR> converter;

	//
	// Dll lists
	//

	std::vector<std::string> dll_list;
	std::vector<LPCSTR> dll_list_str;

	//
	// Argument array
	//

	std::vector<std::string> args;

	//
	// Argument dry run
	//

	bool dry_run = false;

	//
	// Argument parser
	//

	argparse::ArgumentParser program("SunnyDoll");

	//
	// Convert argv to vector and UTF-8
	//

	std::transform(argv, argv + argc, std::back_inserter(args), [&converter](PWSTR unencoded_arg) {
		auto encoded_arg = converter.to_bytes(unencoded_arg);

	spdlog::trace("Argument: {}", encoded_arg);

	return encoded_arg;
		});

	//
	// Argument "program" is the target program path
	//

	program.add_argument("--program", "-p")
		.required()
		.metavar("PATH")
		.help("specify target program path.");

	//
	// Argument "program" is the target program path
	//

	program.add_argument("--work-dir", "-w")
		.metavar("PATH")
		.help("specify directory path for executing target process.");

	//
	// Argument "dry run" to debug the config
	//

	program.add_argument("--dry-run")
		.default_value(false)
		.implicit_value(true)
		.help("test and debug the config.")
		.action([&dry_run](const auto&) {
		dry_run = true;
			});

	//
	// Argument "debug" is whether to enable all output
	//

	program.add_argument("--debug", "-d")
		.default_value(false)
		.implicit_value(true)
		.help("enable debug output.")
		.action([](const auto&) {
		spdlog::info("Output level is set to \"trace\"");
	spdlog::set_level(spdlog::level::trace);
			});

	try {
		program.parse_args(args);
	}
	catch (const std::runtime_error& err) {
		//
		// Error occurred parsing args
		//

		std::cerr << err.what() << std::endl;
		std::cerr << program;

		spdlog::error("Invalid parameters, program terminated! ");

		std::exit(ERROR_INVALID_PARAMETER);
	}

	//
	// User commands
	//

	auto target_path = program.get("--program");
	auto work_dir = program.present("--work-dir");

	//
	// Add pre-defined dll to dll list
	//

	dll_list.push_back(std::filesystem::current_path().string() + MMONITOR_NAME);

	//
	// Transform std::string array to LPCSTR array
	//

	std::transform(dll_list.cbegin(), dll_list.cend(), std::back_inserter(dll_list_str),
		[](const std::string& dll_path) {
			return dll_path.c_str();
		});

	spdlog::info("Target program path: {}", target_path);
	if (work_dir.has_value()) spdlog::trace("Work directory path: {}", work_dir.value());
	for (const auto& dll : dll_list_str) {
		spdlog::info("Loading dll: {}", dll);
	}

	CreateConfig(dll_list);

	if (dry_run) {
		return 0;
	}

	//
	// DetourCreateProcessWithDllsW params
	//

	STARTUPINFOW si{};
	PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);

	//
	// Create process and inject dlls
	//

	spdlog::info("SunnyDoll started! ");

	auto ret = DetourCreateProcessWithDllsW(nullptr, converter.from_bytes(target_path).data(),
		nullptr, nullptr, false, CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
		nullptr, work_dir.has_value() ? converter.from_bytes(work_dir.value()).data() : nullptr,
		&si, &pi, static_cast<DWORD>(dll_list_str.size()), dll_list_str.data(), nullptr
	);

	if (!ret) {
		spdlog::error("DetourCreateProcessWithDllsW failed with errorcode: {}, program terminated! ",
			GetLastError());

		std::exit(GetLastError());
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	spdlog::info("Target program exited! Cleaning up. ");

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}
