#include "logger.hpp"
#include "utility/constants.hpp"
#include "utility/hash.hpp"

namespace std
{
   size_t hash<source_location>::operator()(source_location const& location) const noexcept
   {
      size_t const hash_1 = nes::hash(location.file_name());
      size_t const hash_2 = nes::hash(location.line());
      size_t const hash_3 = nes::hash(location.column());
      size_t const hash_4 = nes::hash(location.function_name());

      size_t seed = 0;
      auto const generate = [&seed](size_t const hash)
      {
         seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      };

      generate(hash_1);
      generate(hash_2);
      generate(hash_3);
      generate(hash_4);

      return seed;
   }

   bool equal_to<source_location>::operator()(source_location const& location_a,
      source_location const& location_b) const noexcept
   {
      if (location_a.line() not_eq location_b.line() or
         location_a.column() not_eq location_b.column())
         return false;

      if (location_a.file_name() == location_b.file_name() and
         location_a.function_name() == location_b.function_name())
         return true;

      return
         not std::strcmp(location_a.file_name(), location_b.file_name()) and
         not std::strcmp(location_a.function_name(), location_b.function_name());
   }
}

namespace nes
{
   Logger::~Logger()
   {
      {
         std::lock_guard const lock{ mutex_ };
         run_thread_ = false;
      }

      condition_.notify_one();
   }

   void Logger::log(Payload const& payload)
   {
      std::ostream* output_stream;
      switch (payload.type)
      {
         case Type::INFO:
            output_stream = &std::clog;
            break;

         case Type::WARNING:
            [[fallthrough]];

         case Type::ERROR:
            output_stream = &std::cerr;
            break;

         default:
            output_stream = &std::cout;
      }

      std::string_view esc_sequence;
      switch (payload.type)
      {
         case Type::INFO:
            esc_sequence = "1;36;40";
            break;

         case Type::WARNING:
            esc_sequence = "1;33;40";
            break;

         case Type::ERROR:
            esc_sequence = "1;31;40";
            break;
      }

      std::time_t const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::tm local_time;
      localtime_s(&local_time, &now);
      std::ostringstream time_stream;
      time_stream << std::put_time(&local_time, "%H:%M:%S");

      if constexpr (MINGW)
         *output_stream << std::format(
            "\033[{}m>> {}({})\n[{}]: {}\033[0m\n",
            esc_sequence,
            payload.location.file_name(),
            payload.location.line(),
            time_stream.str(),
            payload.message);
      else
         std::println(*output_stream,
            "\033[{}m>> {}({})\n[{}]: {}\033[0m",
            esc_sequence,
            payload.location.file_name(),
            payload.location.line(),
            time_stream.str(),
            payload.message);
   }

   void Logger::log_once(Payload const& payload)
   {
      if (not location_entries_.insert(payload.location).second)
         return;

      return log(payload);
   }
}