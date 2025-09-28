#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "pch.hpp"

namespace nes
{
   class InstructionPromise;

   class Instruction final
   {
      public:
         using promise_type = InstructionPromise;

         explicit Instruction(std::coroutine_handle<InstructionPromise> handle);
         Instruction(Instruction const&) = delete;
         Instruction(Instruction&& other) noexcept;

         ~Instruction();

         Instruction& operator=(Instruction const&) = delete;
         Instruction& operator=(Instruction&& other) noexcept;

         void resume() const;
         [[nodiscard]] bool continue_execution() const;
         [[nodiscard]] bool done() const;

      private:
         void destroy_handle() const;

         std::coroutine_handle<InstructionPromise> handle_;
   };

   class InstructionPromise final
   {
      public:
         InstructionPromise() = default;
         InstructionPromise(InstructionPromise const&) = default;
         InstructionPromise(InstructionPromise&&) = default;

         ~InstructionPromise() = default;

         InstructionPromise& operator=(InstructionPromise const&) = default;
         InstructionPromise& operator=(InstructionPromise&&) = default;

         static std::suspend_always initial_suspend() noexcept;
         static std::suspend_always final_suspend() noexcept;
         static void unhandled_exception();

         Instruction get_return_object();
         void return_value(bool continue_execution);

         [[nodiscard]] bool continue_execution() const;

      private:
         bool continue_execution_ = true;
   };
}

#endif