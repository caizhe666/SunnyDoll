#pragma once
#include "boost/interprocess/allocators/allocator.hpp"
#include "boost/interprocess/managed_shared_memory.hpp"
#include "boost/interprocess/containers/string.hpp"
#include "boost/interprocess/containers/vector.hpp"

constexpr auto SHARED_MEMORY_SIZE = 65536;

VOID
CreateConfig(
	const std::vector<std::string>& dll_list
);

VOID
LoadConfigFromArgs(
	VOID
);

VOID
LoadConfigFromFile(
	VOID
);
