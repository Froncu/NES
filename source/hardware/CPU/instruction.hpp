#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include "pch.hpp"

namespace nes
{
   class Instruction final
   {
      struct Promise final
      {
         static std::suspend_always initial_suspend() noexcept;
         static std::suspend_always final_suspend() noexcept;
         static void unhandled_exception();
         static void return_void();

         Instruction get_return_object();
      };

      public:
         using promise_type = Promise;

         explicit Instruction(std::coroutine_handle<Promise> handle);
         Instruction(Instruction const&) = delete;
         Instruction(Instruction&& other) noexcept;

         ~Instruction();

         Instruction& operator=(Instruction const&) = delete;
         Instruction& operator=(Instruction&& other) noexcept;

         void resume() const;
         [[nodiscard]] bool done() const;

      private:
         void destroy_handle() const;

         std::coroutine_handle<Promise> handle_;
   };
}

#endif