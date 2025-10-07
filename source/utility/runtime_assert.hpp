#ifndef RUNTIME_ASSERT_HPP
#define RUNTIME_ASSERT_HPP

#include "constants.hpp"
#include "services/locator.hpp"
#include "services/logger/logger.hpp"

namespace nes
{
   template <typename Message>
   constexpr void runtime_assert(bool const condition, Message&& message, std::source_location location = std::source_location::current())
   {
      if constexpr (DEBUG)
      {
         if (condition)
            return;

         Locator::get<Logger>()->error(std::forward<Message>(message), false, std::move(location));
         std::abort();
      }
   }
}

#endif