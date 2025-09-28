#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

namespace nes
{
   #ifdef NDEBUG
   bool constexpr DEBUG = false;
   #else
   bool constexpr DEBUG = true;
   #endif
}

#endif