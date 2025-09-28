#ifndef UNSUPPORTED_INSTRUCTION_HPP
#define UNSUPPORTED_INSTRUCTION_HPP

#include "emulator_exception.hpp"

namespace nes
{
   class UnsupportedInstruction final : public EmulatorException
   {
      public:
         explicit UnsupportedInstruction(std::uint8_t instruction, std::uint16_t program_counter);
         UnsupportedInstruction(UnsupportedInstruction const&) = default;
         UnsupportedInstruction(UnsupportedInstruction&&) = default;

         virtual ~UnsupportedInstruction() override = default;

         UnsupportedInstruction& operator=(UnsupportedInstruction const&) = delete;
         UnsupportedInstruction& operator=(UnsupportedInstruction&&) = delete;

         std::uint8_t const instruction;
   };
}

#endif