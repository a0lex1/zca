#pragma once

// This requires C++17

#include <bitset>

namespace co {

// https://stackoverflow.com/questions/1448396/how-to-use-enums-as-flags-in-c << some of their nerds offered

// The thing itself:
template <typename EnumT>
class Flags {
  static_assert(std::is_enum_v<EnumT>, "Flags can only be specialized for enum types");

  using UnderlyingT = typename std::make_unsigned_t<typename std::underlying_type_t<EnumT>>;

public:
  Flags& set(EnumT e, bool value = true) noexcept {
    bits_.set(underlying(e), value);
    return *this;
  }

  Flags& reset(EnumT e) noexcept {
    set(e, false);
    return *this;
  }

  Flags& reset() noexcept {
    bits_.reset();
    return *this;
  }

  [[nodiscard]] bool all() const noexcept {
    return bits_.all();
  }

  [[nodiscard]] bool any() const noexcept {
    return bits_.any();
  }

  [[nodiscard]] bool none() const noexcept {
    return bits_.none();
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept {
    return bits_.size();
  }

  [[nodiscard]] std::size_t count() const noexcept {
    return bits_.count();
  }

  constexpr bool operator[](EnumT e) const {
    return bits_[underlying(e)];
  }

private:
  static constexpr UnderlyingT underlying(EnumT e) {
    return static_cast<UnderlyingT>(e);
  }

private:
  std::bitset<underlying(EnumT::size)> bits_;
};

// --------------
/*
// Example of use:
enum class CarState : std::uint8_t {
  engine_on, // 0
  lights_on, // 1
  wipers_on, // 2
  // ...
  size
};

static void foobar() {
  using CarStates = Flags<CarState>;

  CarStates car_states;
  car_states.set(CarState::engine_on);

  if (car_states[CarState::engine_on]) {
    //std::cout << "We are ready to go!" << std::endl;
  }

  if (car_states[CarState::wipers_on]) {
    //std::cout << "Oh, it's raining!" << std::endl;
  }
}

*/

}

