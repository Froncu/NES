#ifndef EMULATION_EXCEPTION_HPP
#define EMULATION_EXCEPTION_HPP

#include "emulator_exception.hpp"

namespace nes
{
   class EmulationException : public EmulatorException
   {
      public:
         explicit EmulationException(std::string what, std::uint16_t program_counter);
         EmulationException(EmulationException const&) = default;
         EmulationException(EmulationException&&) = default;

         virtual ~EmulationException() override = default;

         EmulationException& operator=(EmulationException const&) = delete;
         EmulationException& operator=(EmulationException&&) = delete;

         std::uint16_t const program_counter;
   };
}

#endif