#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "hardware/types.hpp"
#include "pch.hpp"

namespace nes
{
   class Memory final
   {
      public:
         Memory() noexcept = default;
         Memory(Memory const&) = delete;
         Memory(Memory&&) = delete;

         ~Memory() noexcept = default;

         Memory& operator=(Memory const&) = delete;
         Memory& operator=(Memory&&) = delete;

         void load_program(std::filesystem::path const& path, Word load_address = 0x0000) noexcept;

         void write(Word address, Byte data) noexcept;
         [[nodiscard]] Byte read(Word address) const noexcept;

         [[nodiscard]] std::size_t size() const noexcept;

      private:
         std::array<Byte, std::numeric_limits<ProgramCounter>::max() + 1> data_{};
   };
}

#endif