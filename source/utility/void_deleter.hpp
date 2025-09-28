#ifndef VOID_DELETER_HPP
#define VOID_DELETER_HPP

namespace nes
{
   template <typename Type>
   static void void_deleter(void* const value)
   {
      delete static_cast<Type* const>(value);
   }
}

#endif