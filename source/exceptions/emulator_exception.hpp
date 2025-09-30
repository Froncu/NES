#ifndef EMULATOR_EXCEPTION_HPP
#define EMULATOR_EXCEPTION_HPP

#include "pch.hpp"

namespace nes
{
   struct EmulatorException : std::runtime_error
   {
      using std::runtime_error::runtime_error;
   };
}

#endif