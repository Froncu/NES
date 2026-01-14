#include "memory.hpp"

namespace nes
{
   void Memory::load_program(std::filesystem::path const& path, Word const load_address) noexcept
   {
      std::ifstream in{ path.c_str(), std::ios::binary };
      in.read(reinterpret_cast<char*>(&data_[load_address]), data_.size() - load_address);
   }

   void Memory::write(Word const address, Byte const data) noexcept
   {
      data_[address] = data;
   }

   Byte Memory::read(Word const address) const noexcept
   {
      return data_[address];
   }

   std::size_t Memory::size() const noexcept
   {
      return data_.size();
   }
}