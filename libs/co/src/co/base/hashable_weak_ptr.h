#pragma once

#include <memory>

namespace co {
// Public inheritance was used to avoid having to
// duplicate the rest of the API. Unfortunately this
// allows object slicing. So, an alternate solution is
// to use private inheritance, and `using` to provide
// the missing API.
template<class T>
struct hashable_weak_ptr : public std::weak_ptr<T>
{
   hashable_weak_ptr(std::shared_ptr<T>const& sp) :
      std::weak_ptr<T>(sp)
   {
      if (!sp) return;
      _hash = std::hash<T*>{}(sp.get());
   }

   std::size_t get_hash() const noexcept { return _hash; }

   // Define operator<() in order to construct operator==()
   // It might be more efficient to store the unhashed
   // pointer, and use that for equality compares...
   friend bool operator<(hashable_weak_ptr const& lhs,
                         hashable_weak_ptr const& rhs)
   {
      return lhs.owner_before(rhs);
   }
   friend bool operator!=(hashable_weak_ptr const& lhs,
                          hashable_weak_ptr const& rhs)
   {
      return lhs<rhs or rhs<lhs;
   }
   friend bool operator==(hashable_weak_ptr const& lhs,
                          hashable_weak_ptr const& rhs)
   {
      return not (lhs != rhs);
   }
   private:
      std::size_t _hash = 0;
};
} // namespace co

namespace std
{

// Specializations in std namespace needed
// for above to be usable.
template<class T>
struct owner_less<co::hashable_weak_ptr<T>>
{
   bool operator()(const co::hashable_weak_ptr<T>& lhs,
                   const co::hashable_weak_ptr<T>& rhs) const noexcept
   {
      return lhs.owner_before(rhs);
   }
};

template<class T>
struct hash<co::hashable_weak_ptr<T>>
{
   std::size_t operator()(const co::hashable_weak_ptr<T>& w) const noexcept
   {
      return w.get_hash();
   }
};
} // namespace std

