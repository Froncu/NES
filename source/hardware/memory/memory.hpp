#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "hardware/types.hpp"
#include "pch.hpp"

namespace nes
{
   class Memory final
   {
      public:
         Memory() = default;
         Memory(Memory const&) = delete;
         Memory(Memory&&) = delete;

         ~Memory() = default;

         Memory& operator=(Memory const&) = delete;
         Memory& operator=(Memory&&) = delete;
         [[nodiscard]] Data& operator[](Address address);

         void write(Address address, Data data);
         [[nodiscard]] Data read(Address address) const;
         [[nodiscard]] std::size_t size() const;

      private:
         std::array<Data, std::numeric_limits<ProgramCounter>::max() + 1> data_{};
   };
}

#endif