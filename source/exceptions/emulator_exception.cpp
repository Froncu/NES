#include "emulator_exception.hpp"

namespace nes
{
   EmulatorException::EmulatorException(std::string what, std::source_location location)
      : std::runtime_error{ std::move(what) }
      , location_{ std::move(location) }
   {
   }

   std::source_location const& EmulatorException::location() const
   {
      return location_;
   }
}