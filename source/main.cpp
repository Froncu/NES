#include "hardware/CPU/CPU.hpp"

int main()
{
   std::array<std::uint8_t, 65'536> memory{};
   memory[0xFFFD] = 0x06;
   memory[0xFFFC] = 0x00;

   nes::CPU processor{ memory };

   bool continue_execution = true;
   while (continue_execution)
      continue_execution = processor.tick();

   std::println("completed {} cycles!", processor.cycle());
}