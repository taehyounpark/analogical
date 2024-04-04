#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "action.h"

namespace queryosity {

template <typename T> class column::evaluator {

public:
  using evaluated_type = T;

public:
  template <typename... Args> evaluator(Args const &...args);
  virtual ~evaluator() = default;

  template <typename... Vals>
  std::unique_ptr<T> evaluate(view<Vals> const &...cols) const;

protected:
  std::function<std::unique_ptr<T>()> m_make_unique_column;
};

} // namespace queryosity

template <typename T>
template <typename... Args>
queryosity::column::evaluator<T>::evaluator(Args const &...args)
    : m_make_unique_column(std::bind(
          [](Args const &...args) { return std::make_unique<T>(args...); },
          args...)) {}

template <typename T>
template <typename... Vals>
std::unique_ptr<T> queryosity::column::evaluator<T>::evaluate(
    view<Vals> const &...columns) const {
  auto defn = m_make_unique_column();
  defn->set_arguments(columns...);
  return defn;
}