#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "action.h"

namespace ana {

namespace column {

class node : public action {};

class computation;

template <typename T> class reader;

template <typename T> class fixed;

template <typename T> class calculation;

template <typename T> class definition;

template <typename T> class equation;

template <typename T> class representation;

template <typename T> class constant;

template <typename T> class expression;

template <typename T> class evaluate;

template <typename T>
constexpr std::true_type check_reader(typename column::reader<T> const &);
constexpr std::false_type check_reader(...);

template <typename T>
constexpr std::true_type check_fixed(typename column::fixed<T> const &);
constexpr std::false_type check_fixed(...);

template <typename T>
constexpr std::true_type
check_definition(typename column::definition<T> const &);
constexpr std::false_type check_definition(...);

template <typename T>
constexpr std::true_type check_equation(typename column::equation<T> const &);
constexpr std::false_type check_equation(...);

template <typename T>
constexpr std::true_type
check_representation(typename column::representation<T> const &);
constexpr std::false_type check_representation(...);

template <typename T>
constexpr bool is_reader_v =
    decltype(check_reader(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_fixed_v =
    decltype(check_fixed(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_definition_v =
    decltype(check_definition(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_equation_v =
    decltype(check_equation(std::declval<std::decay_t<T> const &>()))::value;

template <typename T>
constexpr bool is_representation_v = decltype(check_representation(
    std::declval<std::decay_t<T> const &>()))::value;

template <typename T> struct is_evaluate : std::false_type {};
template <typename T>
struct is_evaluate<column::evaluate<T>> : std::true_type {};

template <typename T> constexpr bool is_evaluatable_v = is_evaluate<T>::value;

// equation traits

template <typename F> struct equation_traits;

template <typename Ret, typename... Args>
struct equation_traits<std::function<Ret(Args...)>> {
  using equation_type =
      column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>;
};

template <typename Expr> struct equation_traits<expression<Expr>> {
  using function_type = typename expression<Expr>::function_type;
  using equation_type = typename equation_traits<function_type>::equation_type;
};

template <typename F>
using equation_t = typename equation_traits<F>::equation_type;

// evaluate traits
template <typename T, typename = void> struct evaluate_traits;

template <typename T>
struct evaluate_traits<
    T, typename std::enable_if_t<ana::column::template is_definition_v<T>>> {
  using evaluate_type = typename column::template evaluate<T>;
};

template <typename F>
struct evaluate_traits<
    F, typename std::enable_if_t<!ana::column::template is_definition_v<F>>> {
  using evaluate_type = typename ana::column::template evaluate<
      ana::column::template equation_t<F>>;
};

template <typename T>
using evaluate_t = typename evaluate_traits<T>::evaluate_type;

template <typename T> using evaluated_t = typename T::evaluated_type;

} // namespace column

template <typename T>
constexpr bool is_column_v = std::is_base_of_v<column::node, T>;

//---------------------------------------------------
// cell can actually report on the concrete data type
//---------------------------------------------------
template <typename T> class cell {

public:
  using value_type = T;

public:
  template <typename U> class conversion_of;

  template <typename U> class interface_of;

public:
  cell() = default;
  virtual ~cell() = default;

  virtual T const &value() const = 0;
  virtual T const *field() const;
};

//------------------------------------
// conversion between compatible types
//------------------------------------
template <typename To>
template <typename From>
class cell<To>::conversion_of : public cell<To> {

public:
  conversion_of(cell<From> const &from);
  virtual ~conversion_of() = default;

public:
  virtual const To &value() const override;

private:
  cell<From> const *m_from;
  mutable To m_conversion_of;
};

//------------------------------------------
// interface between inherited -> base type
//------------------------------------------
template <typename To>
template <typename From>
class cell<To>::interface_of : public cell<To> {

public:
  interface_of(cell<From> const &from);
  virtual ~interface_of() = default;

public:
  virtual const To &value() const override;

private:
  cell<From> const *m_impl;
};

template <typename T> class term : public column::node, public cell<T> {

public:
  using value_type = typename cell<T>::value_type;

public:
  term() = default;
  virtual ~term() = default;

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) override;
  virtual void execute(unsigned int slot, unsigned long long entry) override;
  virtual void finalize(unsigned int slot) override;
};

// costly to move around
template <typename T> class variable {

public:
  variable() = default;
  template <typename U> variable(cell<U> const &val);
  virtual ~variable() = default;

  variable(variable &&) = default;
  variable &operator=(variable &&) = default;

  T const &value() const;
  T const *field() const;

protected:
  std::unique_ptr<const cell<T>> m_cell;
};

// easy to move around
template <typename T> class observable {

public:
  observable(variable<T> const &obs);
  virtual ~observable() = default;

  T const &value() const;
  T const *field() const;

  T const &operator*() const;
  T const *operator->() const;

protected:
  const variable<T> *m_var;
};

template <typename To, typename From>
std::unique_ptr<cell<To>> cell_as(cell<From> const &from);

template <typename T>
using cell_value_t = std::decay_t<decltype(std::declval<T>().value())>;

} // namespace ana

template <typename T>
void ana::term<T>::initialize(unsigned int, unsigned long long,
                              unsigned long long) {}

template <typename T>
void ana::term<T>::execute(unsigned int, unsigned long long) {}

template <typename T> void ana::term<T>::finalize(unsigned int) {}

template <typename T> T const *ana::cell<T>::field() const {
  return &this->value();
}

template <typename To>
template <typename From>
ana::cell<To>::conversion_of<From>::conversion_of(cell<From> const &from)
    : m_from(&from) {}

template <typename To>
template <typename From>
const To &ana::cell<To>::conversion_of<From>::value() const {
  m_conversion_of = m_from->value();
  return m_conversion_of;
}

template <typename Base>
template <typename Impl>
ana::cell<Base>::interface_of<Impl>::interface_of(cell<Impl> const &from)
    : m_impl(&from) {}

template <typename Base>
template <typename Impl>
const Base &ana::cell<Base>::interface_of<Impl>::value() const {
  return m_impl->value();
}

template <typename To, typename From>
std::unique_ptr<ana::cell<To>> ana::cell_as(cell<From> const &from) {
  static_assert(std::is_same_v<From, To> || std::is_base_of_v<To, From> ||
                    std::is_convertible_v<From, To>,
                "incompatible value types");
  if constexpr (std::is_same_v<From, To> || std::is_base_of_v<To, From>) {
    return std::make_unique<
        typename ana::cell<To>::template interface_of<From>>(from);
  } else if constexpr (std::is_convertible_v<From, To>) {
    return std::make_unique<
        typename ana::cell<To>::template conversion_of<From>>(from);
  }
}

// --------
// variable
// --------

template <typename T>
template <typename U>
ana::variable<T>::variable(cell<U> const &val) : m_cell(cell_as<T>(val)) {}

template <typename T> T const &ana::variable<T>::value() const {
  return m_cell->value();
}

template <typename T> T const *ana::variable<T>::field() const {
  return m_cell->field();
}

template <typename T>
ana::observable<T>::observable(const variable<T> &var) : m_var(&var) {}

template <typename T> T const &ana::observable<T>::operator*() const {
  return m_var->value();
}

template <typename T> T const *ana::observable<T>::operator->() const {
  return m_var->field();
}

template <typename T> T const &ana::observable<T>::value() const {
  return m_var->value();
}

template <typename T> T const *ana::observable<T>::field() const {
  return m_var->field();
}