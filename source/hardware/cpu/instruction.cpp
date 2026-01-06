#include "instruction.hpp"

namespace nes
{
   Instruction::Instruction(std::coroutine_handle<promise_type> handle)
      : handle_{ std::move(handle) }
   {
   }

   Instruction::Instruction(Instruction&& other) noexcept
      : handle_{ std::exchange(other.handle_, nullptr) }
   {
   }

   Instruction::~Instruction()
   {
      destroy_handle();
   }

   Instruction& Instruction::operator=(Instruction&& other) noexcept
   {
      destroy_handle();
      handle_ = std::exchange(other.handle_, nullptr);
      return *this;
   }

   bool Instruction::tick() const
   {
      handle_.resume();
      return handle_.done();
   }

   std::optional<Instruction>&& Instruction::prefetched_instruction() const
   {
      return std::move(handle_.promise().prefetched_instruction);
   }

   void Instruction::destroy_handle() const
   {
      if (handle_)
         handle_.destroy();
   }

   std::suspend_always Instruction::promise_type::initial_suspend() noexcept
   {
      return {};
   }

   std::suspend_always Instruction::promise_type::final_suspend() noexcept
   {
      return {};
   }

   void Instruction::promise_type::unhandled_exception()
   {
   }

   void Instruction::promise_type::return_value(std::optional<Instruction> instruction)
   {
      prefetched_instruction = std::move(instruction);
   }

   Instruction Instruction::promise_type::get_return_object()
   {
      return Instruction{ std::coroutine_handle<promise_type>::from_promise(*this) };
   }
}