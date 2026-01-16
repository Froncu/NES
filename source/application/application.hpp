#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "application/application.hpp"
#include "exceptions/unsupported_opcode.hpp"
#include "hardware/memory/memory.hpp"
#include "hardware/processor/processor.hpp"
#include "services/locator.hpp"
#include "services/visualiser/visualiser.hpp"

namespace nes
{
   class Application final
   {
      public:
         Application() noexcept = default;
         Application(Application const&) = delete;
         Application(Application&&) = delete;

         ~Application() noexcept = default;

         Application& operator=(Application const&) = delete;
         Application& operator=(Application&&) = delete;

         bool update();

      private:
         void handle_exception(UnsupportedOpcode const& exception,
            std::source_location source_location = std::source_location::current()) const;

         void try_tick();
         void try_tick_repeatedly(std::stop_token const& stop_token);
         void try_step();

         Visualiser& visualiser_{ *Locator::get<Visualiser>() };
         Logger& logger_{ *Locator::get<Logger>() };

         Memory memory_{};
         Processor processor_{ memory_ };
         std::jthread emulation_thread_{};
   };
}

#endif