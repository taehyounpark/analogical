#pragma once

#include <string>
#include <memory>
#include <functional>

#include "ana/column.h"
#include "ana/term.h"
#include "ana/calculation.h"

namespace ana
{

template <typename Ret>
template <typename... Args>
class term<Ret>::evaluated_from : public term<Ret>::calculation
{

public:
  using argtuple_type = std::tuple<std::shared_ptr<cell<Args>>...>;
  using evalfunc_type = std::function<Ret(const Args&...)>;

public:
  evaluated_from();
	virtual ~evaluated_from() = default;

  template <typename F>
	void set_evaluation(F&& callable);

  // convert each input argument type
  template <typename... UArgs>
  void set_arguments(cell<UArgs>&... args);

  virtual Ret calculate() const override;

  auto get_arguments() const -> argtuple_type;

protected:
	argtuple_type m_arguments;
	evalfunc_type m_evalute;

};

template <typename Ret, typename... Args>
std::shared_ptr<column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>> make_equation(std::function<Ret(Args...)> func)
{
	(void)(func);
	auto eqn = std::make_shared<column::equation<std::decay_t<Ret>(std::decay_t<Args>...)>>();
	return eqn;
}

template <typename Ret, typename... Args>
class column::equation<Ret(Args...)> : public term<Ret>::template evaluated_from<Args...>
{
public:
  equation();
	virtual ~equation() = default;
};

template <typename F>
using equation_t = typename decltype(make_equation(std::function(std::declval<F>())))::element_type;

template <typename T>
struct is_column_equation : std::false_type {};
template <typename T>
struct is_column_equation<column::equation<T>> : std::true_type {};
template <typename T>
constexpr bool is_column_equation_v = is_column_equation<T>::value;

}

template <typename Ret>
template <typename... Args>
ana::term<Ret>::evaluated_from<Args...>::evaluated_from() :
	term<Ret>::calculation()
{}

template <typename Ret, typename... Args>
ana::column::equation<Ret(Args...)>::equation() :
	term<Ret>::template evaluated_from<Args...>()
{}

template <typename Ret>
template <typename... Args>
template <typename F>
void ana::term<Ret>::evaluated_from<Args...>::set_evaluation(F&& callable)
{
	m_evalute = std::function<Ret(const Args&...)>(std::forward<F>(callable));
}

template <typename Ret>
template <typename... Args>
template <typename... UArgs>
void ana::term<Ret>::evaluated_from<Args...>::set_arguments(cell<UArgs>&... args)
{
  static_assert(sizeof...(Args)==sizeof...(UArgs));
  m_arguments = std::make_tuple(
    std::invoke(
      [](cell<UArgs>& args) -> std::shared_ptr<cell<Args>> {
        return ana::cell_as<Args>(args);
    },args)...
  );
}

template <typename Ret>
template <typename... Args>
auto ana::term<Ret>::evaluated_from<Args...>::get_arguments() const -> argtuple_type
{
  return m_arguments;
}

// user-defined evaluation with input arguments
template <typename Ret>
template <typename... Args>
Ret ana::term<Ret>::evaluated_from<Args...>::calculate() const
{
  return std::apply(
    [this](const std::shared_ptr<cell<Args>>&... args) { 
      return this->m_evalute(args->value()...);
    },m_arguments
  );
}