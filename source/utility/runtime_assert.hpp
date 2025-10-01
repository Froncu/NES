#ifndef RUNTIME_ASSERT_HPP
#define RUNTIME_ASSERT_HPP

#include "constants.hpp"
#include "services/locator.hpp"
#include "services/logger/logger.hpp"

namespace nes
{
   template <std::stacktrace::size_type StackTraceDepth = 0, typename... Arguments>
   constexpr void runtime_assert([[maybe_unused]] bool const condition,
      [[maybe_unused]] std::format_string<Arguments...> const format,
      [[maybe_unused]] Arguments&&... arguments)
   {
      if constexpr (DEBUG)
      {
         if (condition)
            return;

         Locator::get<Logger>()->error<StackTraceDepth + 1>(format, std::forward<Arguments>(arguments)...);
         std::abort();
      }
   }

   template <typename Message>
   constexpr void runtime_assert(bool const condition, Message&& message)
   {
      runtime_assert<1>(condition, "{}", std::forward<Message>(message));
   }
}

#endif