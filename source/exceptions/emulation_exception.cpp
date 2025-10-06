#include "emulation_exception.hpp"

namespace nes
{
   EmulationException::EmulationException(ProgramCounter const program_counter, std::string what, std::source_location location)
      : EmulatorException{ std::move(what), std::move(location) }
      , program_counter{ program_counter }
   {
   }

   EmulationException::EmulationException(ProgramCounter const program_counter)
      : EmulationException{ program_counter, std::format("encountered an emulation error at 0x{:04X}", program_counter) }
   {
   }
}