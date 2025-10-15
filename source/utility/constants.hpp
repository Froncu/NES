#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

namespace nes
{
   #ifdef NDEBUG
   auto constexpr DEBUG{ false };
   #else
   auto constexpr DEBUG{ true };
   #endif

   #ifdef __MINGW32__
   auto constexpr MINGW{ true };
   #else
   auto constexpr MINGW{ false };
   #endif
}

#endif