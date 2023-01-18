#include "stdafx.h"
#include "ConfigProvider.h"

//
// Config file support
//

#include "pugixml.hpp"

//
// Time support
//

#include <chrono>

VOID
CreateConfig(
	const std::vector<std::string>& dll_list
)
{
	pugi::xml_document config_file;

	//
	// Write the generic header
	//

	auto root = config_file.append_child("SunnyDoll");

	//
	// Write the version
	//

	root.append_attribute("Version").set_value(SUNNYDOLL_VERSION);

	//
	// Write create time
	//

	const auto current = std::chrono::system_clock::now();

	root.append_attribute("TimeStamp").set_value(
		std::chrono::duration_cast<std::chrono::milliseconds>(
			current.time_since_epoch()).count());

	//
	// Write author
	//

	root.append_attribute("Author").set_value("Mitsuha");

	//
	// Write dll list
	//

	auto dll_node = root.append_child("DllList");

	for (const auto& item : dll_list) {
		std::filesystem::path dll_path(item);

		dll_node.append_child(dll_path.filename().string().c_str()).append_attribute("Path").set_value(item.c_str());
	}

	config_file.print(std::cout);
}
