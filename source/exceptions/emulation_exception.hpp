#ifndef EMULATION_EXCEPTION_HPP
#define EMULATION_EXCEPTION_HPP

#include "emulator_exception.hpp"
#include "hardware/types.hpp"

namespace nes
{
   class EmulationException : public EmulatorException
   {
      public:
         EmulationException(ProgramCounter program_counter, std::string what);
         explicit EmulationException(ProgramCounter program_counter);
         EmulationException(EmulationException const&) = default;
         EmulationException(EmulationException&&) = default;

         virtual ~EmulationException() override = default;

         EmulationException& operator=(EmulationException const&) = delete;
         EmulationException& operator=(EmulationException&&) = delete;

         ProgramCounter const program_counter;
   };
}

#endif