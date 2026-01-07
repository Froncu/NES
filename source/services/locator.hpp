// Inspired by Matias Devred's locator from his "gooey" engine
// (https://git.allpurposem.at/mat/gooey/src/branch/main/src/lib/services/locator.cppm)

#ifndef LOCATOR_HPP
#define LOCATOR_HPP

#include "pch.hpp"
#include "utility/type_index.hpp"
#include "utility/unique_pointer.hpp"
#include "utility/void_deleter.hpp"

namespace nes
{
   class Locator final
   {
      public:
         template <typename Service, std::derived_from<Service> Provider = Service, typename... Arguments>
            requires std::constructible_from<Provider, Arguments...>
         static Service& provide(Arguments&&... arguments) noexcept
         {
            UniquePointer<void> new_provider{ new Provider{ std::forward<Arguments>(arguments)... }, void_deleter<Provider> };

            auto&& [service_index, did_insert]{ service_indices_.emplace(type_index<Service>(), services_.size()) };
            if (did_insert)
               return *static_cast<Service*>(services_.emplace_back(std::move(new_provider)).get());

            UniquePointer<void>& current_provider{ services_[service_index->second] };

            if constexpr (std::movable<Service>)
               *static_cast<Service*>(new_provider.get()) = std::move(*static_cast<Service*>(current_provider.get()));

            current_provider = std::move(new_provider);
            return *static_cast<Service*>(current_provider.get());
         }

         static void remove_providers() noexcept
         {
            while (not services_.empty())
               services_.pop_back();

            service_indices_.clear();
         }

         template <typename Service>
         [[nodiscard]] static Service* get() noexcept
         {
            auto const service_index{ service_indices_.find(type_index<Service>()) };
            if (service_index == service_indices_.end())
               return nullptr;

            return static_cast<Service* const>(services_[service_index->second].get());
         }

         Locator() = delete;
         Locator(Locator const&) = delete;
         Locator(Locator&&) = delete;

         ~Locator() = delete;

         Locator& operator=(Locator const&) = delete;
         Locator& operator=(Locator&&) = delete;

      private:
         static std::unordered_map<std::type_index, std::size_t> service_indices_;
         static std::vector<UniquePointer<void>> services_;
   };
}

#endif