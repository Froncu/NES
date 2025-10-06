#ifndef EMULATOR_EXCEPTION_HPP
#define EMULATOR_EXCEPTION_HPP

#include "pch.hpp"

namespace nes
{
   class EmulatorException : public std::runtime_error
   {
      public:
         explicit EmulatorException(std::string what, std::source_location location = std::source_location::current());
         EmulatorException(EmulatorException const&) = default;
         EmulatorException(EmulatorException&&) = default;

         virtual ~EmulatorException() override = default;

         EmulatorException& operator=(EmulatorException const&) = default;
         EmulatorException& operator=(EmulatorException&&) = default;

         [[nodiscard]] std::source_location const& location() const;

      private:
         std::source_location location_;
   };
}

#endif