#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "hardware/types.hpp"
#include "pch.hpp"

namespace nes
{
   using Memory = std::array<Data, std::numeric_limits<ProgramCounter>::max() + 1>;
}

#endif