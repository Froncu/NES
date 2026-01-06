#include "memory.hpp"

void nes::Memory::load_program(std::filesystem::path const& path, Address const load_address)
{
   std::ifstream in{ path.c_str(), std::ios::binary };
   in.read(reinterpret_cast<char*>(&data_[load_address]), data_.size() - load_address);
}

void nes::Memory::write(Address const address, Data const data)
{
   data_[address] = data;
}

nes::Data nes::Memory::read(Address const address) const
{
   return data_[address];
}

std::size_t nes::Memory::size() const
{
   return data_.size();
}