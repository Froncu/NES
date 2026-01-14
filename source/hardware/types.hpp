#ifndef TYPES_HPP
#define TYPES_HPP

#include "pch.hpp"

namespace nes
{
   using Byte = std::uint8_t;
   using SignedByte = std::int8_t;
   using Word = std::uint16_t;
   using Cycle = std::uint64_t;

   using ProgramCounter = Word;
   using Accumulator = Byte;
   using Index = Byte;
   using StackPointer = Byte;
   using ProcessorStatus = Byte;
}

#endif