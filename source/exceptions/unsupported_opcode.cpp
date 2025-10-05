#include "unsupported_opcode.hpp"

namespace nes
{
   UnsupportedOpcode::UnsupportedOpcode(ProgramCounter const program_counter, Data const opcode)
      : EmulationException{
         program_counter,
         std::format("encountered an unsupported 0x{:02X} opcode at 0x{:04X}", opcode, program_counter)
      }
      , opcode{ opcode }
   {
   }
}