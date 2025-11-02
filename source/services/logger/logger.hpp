#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "pch.hpp"

namespace std
{
   template <>
   struct hash<source_location>
   {
      [[nodiscard]] std::size_t operator()(source_location const& location) const noexcept;
   };

   template <>
   struct equal_to<source_location>
   {
      [[nodiscard]] bool operator()(source_location const& location_a, source_location const& location_b) const noexcept;
   };
}

namespace nes
{
   class Logger final
   {
      enum class Type
      {
         INFO,
         WARNING,
         ERROR
      };

      struct Payload final
      {
         Type type;
         std::source_location location;
         std::string message;
      };

      struct LogInfo final
      {
         bool once;
         Payload payload;
      };

      public:
         Logger() = default;
         Logger(Logger const&) = delete;
         Logger(Logger&&) = delete;

         ~Logger();

         Logger& operator=(Logger const&) = delete;
         Logger& operator=(Logger&&) = delete;

         template <typename Message>
         void info(Message&& message, bool const once = false, std::source_location location = std::source_location::current())
         {
            {
               std::lock_guard const lock{ mutex_ };
               log_queue_.push({
                  .once{ once },
                  .payload{
                     .type{ Type::INFO },
                     .location{ std::move(location) },
                     .message{ std::format("{}", message) }
                  }
               });
            }

            condition_.notify_one();
         }

         template <typename Message>
         void warning(Message&& message, bool const once = false,
            std::source_location location = std::source_location::current())
         {
            {
               std::lock_guard const lock{ mutex_ };
               log_queue_.push({
                  .once{ once },
                  .payload{
                     .type{ Type::WARNING },
                     .location{ std::move(location) },
                     .message{ std::format("{}", message) }
                  }
               });
            }

            condition_.notify_one();
         }

         template <typename Message>
         void error(Message&& message, bool const once = false, std::source_location location = std::source_location::current())
         {
            {
               std::lock_guard const lock{ mutex_ };
               log_queue_.push({
                  .once{ once },
                  .payload{
                     .type = Type::ERROR,
                     .location{ std::move(location) },
                     .message{ std::format("{}", message) }
                  }
               });
            }

            condition_.notify_one();
         }

      private:
         static void log(Payload const& payload);
         void log_once(Payload const& payload);

         std::unordered_set<std::source_location> location_entries_{};

         bool run_thread_{ true };
         std::queue<LogInfo> log_queue_{};

         std::mutex mutex_{};
         std::condition_variable condition_{};
         std::jthread thread_{
            [this]
            {
               while (true)
               {
                  LogInfo log_info;

                  {
                     std::unique_lock lock{ mutex_ };
                     condition_.wait(lock,
                        [this]
                        {
                           return not run_thread_ or not log_queue_.empty();
                        });

                     if (not run_thread_)
                        break;

                     log_info = std::move(log_queue_.front());
                     log_queue_.pop();
                  }

                  if (log_info.once)
                     log_once(log_info.payload);
                  else
                     log(log_info.payload);
               }
            }
         };
   };
}

#endif