#pragma once

#include "selection.h"

namespace ana {

class selection::weight : public selection {

public:
  weight(const selection *presel, bool ch, const std::string &name);
  virtual ~weight() = default;

public:
  virtual bool passed_cut() const override;
  virtual double get_weight() const override;
};

} // namespace ana

inline ana::selection::weight::weight(const selection *presel, bool ch,
                                      const std::string &name)
    : selection(presel, ch, name) {}

inline bool ana::selection::weight::passed_cut() const {
  return m_preselection ? m_preselection->passed_cut() : true;
}
inline double ana::selection::weight::get_weight() const {
  return m_preselection ? m_preselection->get_weight() * m_variable.value()
                        : m_variable.value();
}