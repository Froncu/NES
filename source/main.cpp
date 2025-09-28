#include "exceptions/unsupported_instruction.hpp"
#include "hardware/CPU/CPU.hpp"

int main()
{
   std::array<std::uint8_t, 65'536> memory{};
   memory[0xFFFD] = 0x06;
   memory[0xFFFC] = 0x00;
   memory[0x0600] = 0x01;

   nes::CPU processor{ memory };

   while (true)
      try
      {
         if (not processor.tick())
            break;
      }
      catch (nes::UnsupportedInstruction const& exception)
      {
         std::println("{}", exception.what());
      }

   std::println("completed {} cycles!", processor.cycle());
}