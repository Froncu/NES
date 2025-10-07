#ifndef TYPES_HPP
#define TYPES_HPP

#include "pch.hpp"

namespace nes
{
   using Cycle = std::uint64_t;
   using ProgramCounter = std::uint16_t;
   using Accumulator = std::uint8_t;
   using Index = std::uint8_t;
   using StackPointer = std::uint8_t;
   using ProcessorStatus = std::uint8_t;

   using Data = std::uint8_t;
}

#endif