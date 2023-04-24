#pragma once

#include "ana/term.h"

namespace ana
{

//------------------------------------------------------------------------------
// calculation: value is reset (i.e. updated if needed) per-entry
//------------------------------------------------------------------------------
template <typename T>
class term<T>::calculation : public term<T>
{

public:
	calculation(const std::string& name);
  virtual ~calculation() = default;

public:
  virtual const T& value() const override;

  virtual T calculate() const = 0;

	virtual void initialize() override;
	virtual void execute()    override final;
	virtual void finalize()   override;

protected:
  void update() const;
  void reset();

protected:
  mutable T    m_value;
  mutable bool m_updated;

};

}

template <typename T>
ana::term<T>::calculation::calculation(const std::string& name) :
  term<T>(name),
  m_updated(true)
{}

template <typename T>
const T& ana::term<T>::calculation::value() const
{
  if (!m_updated) this->update();
  return m_value;
}

template <typename T>
void ana::term<T>::calculation::update() const
{
  m_value = this->calculate();
  m_updated = true;
}

template <typename T>
void ana::term<T>::calculation::reset()
{
  m_updated = false;
}

template <typename T>
void ana::term<T>::calculation::initialize()
{}

template <typename T>
void ana::term<T>::calculation::execute()
{ 
  this->reset();
}

template <typename T>
void ana::term<T>::calculation::finalize()
{}