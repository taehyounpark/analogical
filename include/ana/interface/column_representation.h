#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include "column.h"

namespace ana {

/**
 * @brief Representation of multipile columns, out of which derived quantites
 * can be calculated.
 */
template <typename Me, typename... Obs>
class column::representation<Me(Obs...)> : public term<Me> {

public:
  using vartuple_type = std::tuple<variable<Obs>...>;
  using obstuple_type = std::tuple<observable<Obs>...>;

public:
  representation() = default;
  virtual ~representation() = default;

  template <typename... Vals> void set_arguments(const cell<Vals> &...args);

  virtual Me const &value() const override;

  template <unsigned int N>
  constexpr auto value() const
      -> decltype(std::get<N>(std::declval<vartuple_type>()).value());

  template <auto N> constexpr auto value() const;

protected:
  vartuple_type m_arguments;
};

} // namespace ana

template <typename Me, typename... Obs>
Me const &ana::column::representation<Me(Obs...)>::value() const {
  return static_cast<const Me &>(*this);
}

template <typename Me, typename... Obs>
template <typename... Vals>
void ana::column::representation<Me(Obs...)>::set_arguments(
    cell<Vals> const &...args) {
  static_assert(sizeof...(Obs) == sizeof...(Vals));
  m_arguments = std::make_tuple(std::invoke(
      [](const cell<Vals> &args) -> variable<Obs> {
        return variable<Obs>(args);
      },
      args)...);
}

template <typename Me, typename... Obs>
template <unsigned int N>
constexpr auto ana::column::representation<Me(Obs...)>::value() const
    -> decltype(std::get<N>(std::declval<vartuple_type>()).value()) {
  return std::get<N>(m_arguments).value();
}

template <typename Me, typename... Obs>
template <auto N>
constexpr auto ana::column::representation<Me(Obs...)>::value() const {
  constexpr auto idx = static_cast<std::underlying_type_t<decltype(N)>>(N);
  return std::get<idx>(this->m_arguments).value();
}