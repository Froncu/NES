#ifndef EMULATOR_EXCEPTION_HPP
#define EMULATOR_EXCEPTION_HPP

#include "pch.hpp"

namespace nes
{
   class EmulatorException : public std::runtime_error
   {
      public:
         explicit EmulatorException(std::string what, std::uint16_t program_counter);
         EmulatorException(EmulatorException const&) = default;
         EmulatorException(EmulatorException&&) = default;

         virtual ~EmulatorException() override = default;

         EmulatorException& operator=(EmulatorException const&) = delete;
         EmulatorException& operator=(EmulatorException&&) = delete;

         std::uint16_t const program_counter;
   };
}

#endif