#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "pch.hpp"

namespace nes
{
   class Instruction final
   {
      public:
         struct promise_type;

         explicit Instruction(std::coroutine_handle<promise_type> handle);
         Instruction(Instruction const&) = delete;
         Instruction(Instruction&& other) noexcept;

         ~Instruction();

         Instruction& operator=(Instruction const&) = delete;
         Instruction& operator=(Instruction&& other) noexcept;

         [[nodiscard]] bool tick() const;
         [[nodiscard]] std::optional<Instruction>&& prefetched_instruction() const;

      private:
         void destroy_handle() const;

         std::coroutine_handle<promise_type> handle_;
   };

   struct Instruction::promise_type
   {
      static std::suspend_always initial_suspend() noexcept;
      static std::suspend_always final_suspend() noexcept;
      static void unhandled_exception();
      void return_value(std::optional<Instruction> instruction);

      Instruction get_return_object();

      std::optional<Instruction> prefetched_instruction{};
   };
}

#endif