#include "instruction.hpp"

namespace nes
{
   std::suspend_always Instruction::Promise::initial_suspend() noexcept
   {
      return {};
   }

   std::suspend_always Instruction::Promise::final_suspend() noexcept
   {
      return {};
   }

   void Instruction::Promise::unhandled_exception()
   {
      std::terminate();
   }

   Instruction Instruction::Promise::get_return_object()
   {
      return Instruction{ std::coroutine_handle<Promise>::from_promise(*this) };
   }

   void Instruction::Promise::return_value(bool const return_value)
   {
      continue_execution = return_value;
   }

   Instruction::Instruction(std::coroutine_handle<Promise> const handle)
      : handle_{ handle }
   {
   }

   Instruction::Instruction(Instruction&& other) noexcept
      : handle_{ other.handle_ }
   {
      other.handle_ = nullptr;
   }

   Instruction::~Instruction()
   {
      destroy_handle();
   }

   Instruction& Instruction::operator=(Instruction&& other) noexcept
   {
      destroy_handle();
      handle_ = other.handle_;
      other.handle_ = nullptr;
      return *this;
   }

   void Instruction::resume() const
   {
      handle_.resume();
   }

   bool Instruction::continue_execution() const
   {
      return handle_.promise().continue_execution;
   }

   bool Instruction::done() const
   {
      return handle_.done();
   }

   void Instruction::destroy_handle() const
   {
      if (handle_)
         handle_.destroy();
   }
}