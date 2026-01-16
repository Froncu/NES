#include "application.hpp"
#include "services/visualiser/visualiser.hpp"

namespace nes
{
   bool Application::update()
   {
      if (not visualiser_.update(memory_, processor_))
         return false;

      if (visualiser_.tick_repeatedly())
      {
         if (not emulation_thread_.joinable())
            emulation_thread_ = std::jthread{ std::bind_front(&Application::try_tick_repeatedly, this) };
      }
      else if (emulation_thread_.joinable())
      {
         emulation_thread_.request_stop();
         emulation_thread_.join();
      }
      else if (visualiser_.tick_once())
         try_tick();
      else if (visualiser_.step())
         try_step();
      else if (visualiser_.reset())
         processor_.reset();

      if (visualiser_.load_program_requested())
         memory_.load_program(visualiser_.program_path(), visualiser_.program_load_address());

      return true;
   }

   void Application::handle_exception(UnsupportedOpcode const& exception, std::source_location source_location) const
   {
      logger_.error(exception.what(), false, std::move(source_location));
   }

   void Application::try_tick() try
   {
      processor_.tick();
   }
   catch (UnsupportedOpcode const& exception)
   {
      handle_exception(exception);
   }

   void Application::try_tick_repeatedly(std::stop_token const& stop_token)
   {
      while (not stop_token.stop_requested())
         try_tick();
   }

   void Application::try_step() try
   {
      while (not processor_.tick());
   }
   catch (UnsupportedOpcode const& exception)
   {
      handle_exception(exception);
   }
}