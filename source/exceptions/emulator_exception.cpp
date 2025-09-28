#include "emulator_exception.hpp"

namespace nes
{
   EmulatorException::EmulatorException(std::string what, std::uint16_t program_counter)
      : std::runtime_error{ std::move(what) }
      , program_counter{ program_counter }
   {
   }
}