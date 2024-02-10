#pragma once

#include <string>
#include <tuple>

#include "column.h"

namespace ana {

class dataflow;

template <typename Expr> class column::expression {

public:
  expression(Expr expr);
  ~expression() = default;

  auto _equate(dataflow &df) const;

  template <typename Sel> auto _select(dataflow &df) const;

protected:
  Expr m_expression;
};

} // namespace ana

#include "dataflow.h"

template <typename Expr>
ana::column::expression<Expr>::expression(Expr expr) : m_expression(expr) {}

template <typename Expr>
auto ana::column::expression<Expr>::_equate(ana::dataflow &df) const {
  return df._equate(this->m_expression);
}

template <typename Expr>
template <typename Sel>
auto ana::column::expression<Expr>::_select(ana::dataflow &df) const {
  return df._select<Sel>(this->m_expression);
}