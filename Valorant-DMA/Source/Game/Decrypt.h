#pragma once
#include <cstdint>
#include "../DMA/MemoryReader.h"

// Decrypts the UWorld pointer using Valorant's obfuscation routine.
// Isolated in its own file so it can be easily replaced when Riot changes the algorithm.
uintptr_t DecryptUWorld(uintptr_t baseAddress, const MemoryReader& reader);
