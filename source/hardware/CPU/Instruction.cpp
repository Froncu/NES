#include "instruction.hpp"

namespace nes
{
   Instruction::Instruction(std::coroutine_handle<InstructionPromise> const handle)
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
      return handle_.promise().continue_execution();
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

   std::suspend_always InstructionPromise::initial_suspend() noexcept
   {
      return {};
   }

   std::suspend_always InstructionPromise::final_suspend() noexcept
   {
      return {};
   }

   void InstructionPromise::unhandled_exception()
   {
      std::terminate();
   }

   Instruction InstructionPromise::get_return_object()
   {
      return Instruction{ std::coroutine_handle<InstructionPromise>::from_promise(*this) };
   }

   void InstructionPromise::return_value(bool const continue_execution)
   {
      continue_execution_ = continue_execution;
   }

   bool InstructionPromise::continue_execution() const
   {
      return continue_execution_;
   }
}