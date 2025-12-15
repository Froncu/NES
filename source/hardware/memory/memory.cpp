#include "memory.hpp"

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