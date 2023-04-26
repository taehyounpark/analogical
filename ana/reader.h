#pragma once

#include "ana/term.h"

namespace ana
{

template <typename T>
class term<T>::reader : public term<T>
{

public:
  reader(const std::string& name);
  virtual ~reader() = default;

  virtual const T& value() const override;
  void read(const T& val);

protected:
	const T* m_addr;

};

template <typename T>
class column::reader : public term<T>::reader
{

public:
  reader(const std::string& name);
  virtual ~reader() = default;

};

template <typename Val>
constexpr std::true_type check_column_reader(const typename column::reader<Val>&);
constexpr std::false_type check_column_reader(...);
template <typename Val>
constexpr bool is_column_reader_v = decltype(check_column_reader(std::declval<Val>()))::value;

}

template <typename T>
 ana::term<T>::reader::reader(const std::string& name) :
  term<T>(name),
  m_addr(nullptr)
{}

template <typename T>
 ana::column::reader<T>::reader(const std::string& name) :
  term<T>::reader(name)
{}

template <typename T>
void ana::term<T>::reader::read(const T& val)
{
  m_addr = &val;
}

template <typename T>
const T& ana::term<T>::reader::value() const
{
  return *m_addr;
}