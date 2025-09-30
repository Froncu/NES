#include "emulation_exception.hpp"

namespace nes
{
   EmulationException::EmulationException(std::string what, std::uint16_t program_counter)
      : EmulatorException{ std::move(what) }
      , program_counter{ program_counter }
   {
   }
}