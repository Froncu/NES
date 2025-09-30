#include "unsupported_instruction.hpp"

namespace nes
{
   UnsupportedInstruction::UnsupportedInstruction(std::uint16_t const program_counter, std::uint8_t const instruction)
      : EmulationException{
         std::format("encountered an unsupported 0x{:02X} instruction at 0x{:04X}", instruction, program_counter),
         program_counter
      }
      , instruction{ instruction }
   {
   }
}