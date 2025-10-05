#include "logger.hpp"

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

   std::stacktrace_entry Logger::location(std::stacktrace::size_type stack_trace_depth)
   {
      stack_trace_depth += 2;
      auto const stack_trace = std::stacktrace::current(stack_trace_depth, 1);
      return stack_trace[0];
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

      std::string payload_source_file = payload.location.source_file();
      std::string const source_file_location =
         payload_source_file.empty() ? "unknown" : std::format("{}({})", payload_source_file, payload.location.source_line());

      std::time_t const now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::tm local_time;
      localtime_s(&local_time, &now);
      std::ostringstream time_stream;
      time_stream << std::put_time(&local_time, "%H:%M:%S");

      std::println(*output_stream,
         "\033[{}m>> {}\n[{}]: {}\033[0m",
         esc_sequence,
         source_file_location,
         time_stream.str(),
         payload.message);
   }

   void Logger::log_once(Payload const& payload)
   {
      if (payload.location and not stacktrace_entries_.insert(payload.location).second)
         return;

      return log(payload);
   }
}