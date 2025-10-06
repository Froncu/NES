#ifndef UNSUPPORTED_OPCODE_HPP
#define UNSUPPORTED_OPCODE_HPP

#include "emulation_exception.hpp"

namespace nes
{
   class UnsupportedOpcode final : public EmulationException
   {
      public:
         explicit UnsupportedOpcode(ProgramCounter program_counter, Data opcode,
            std::source_location location = std::source_location::current());
         UnsupportedOpcode(UnsupportedOpcode const&) = default;
         UnsupportedOpcode(UnsupportedOpcode&&) = default;

         virtual ~UnsupportedOpcode() override = default;

         UnsupportedOpcode& operator=(UnsupportedOpcode const&) = delete;
         UnsupportedOpcode& operator=(UnsupportedOpcode&&) = delete;

         Data const opcode;
   };
}

#endif