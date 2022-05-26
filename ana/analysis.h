#pragma once

#include <vector>
#include <memory>
#include <string>
#include <type_traits>
#include <functional>
#include <thread>

#include "ana/table.h"
#include "ana/sample.h"
#include "ana/column.h"
#include "ana/cell.h"
#include "ana/selection.h"
#include "ana/cut.h"
#include "ana/weight.h"
#include "ana/counter.h"
#include "ana/concurrent.h"
#include "ana/processor.h"

namespace ana
{

template <typename T>
class analysis : public sample<T>
{

public:
public:
  using dataset_type = typename sample<T>::dataset_type;
  using reader_type = typename sample<T>::reader_type;

public:
	template <typename U>
	class node;

public:
  analysis(std::unique_ptr<T> dataset);
  template <typename... Args>
  analysis(Args&&... args);
  virtual ~analysis() = default;

  template <typename Val, typename... Args>
  node<column<Val>> read(const std::string& name, const Args&... args);

  template <typename Val>
  node<column<Val>> constant(const std::string& name, const Val& value);

  template <typename Def, typename... Args>
  node<Def> define(const std::string& name, const Args&... arguments);

  template <typename F, typename... Vars>
  auto evaluate(const std::string& name, F callable, const node<Vars>&... columns) -> node<column<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>;

  template <typename Sel, typename F, typename... Vars>
  node<selection> filter(const std::string& name, F callable, const node<Vars>&... columns);

  template <typename Sel, typename F, typename... Vars>
  node<selection> channel(const std::string& name, F callable, const node<Vars>&... columns);

  analysis& from(const node<selection>& rebase);
	node<selection> operator[](const std::string& path) const;

	template <typename Cnt, typename... Args>
	node<counter::bookkeeper<Cnt>> count(const std::string& name, const Args&... arguments);

	template <typename Cnt>
	node<Cnt> book(const node<counter::bookkeeper<Cnt>>& bookkeeper);

	std::vector<std::string> list_term_names() const;
	std::vector<std::string> list_selection_paths() const;
	std::vector<std::string> list_counter_paths() const;

	bool has_term(const std::string& name) const;
	bool has_selection(const std::string& path) const;
	bool has_counter(const std::string& path) const;

public:
	void analyze();
	void reset();

	void merge_results();
	void clear_counters();

protected:
  void run_processors();

protected:
	void add_term(node<term> var);
	void add_selection(node<selection> sel);
	void add_counter(node<counter> cnt);

protected:
	bool m_analyzed;

	std::vector<std::string>                   m_column_names;
	std::unordered_map<std::string,node<term>> m_column_map;

	std::vector<std::string>                        m_selection_paths;
	std::unordered_map<std::string,node<selection>> m_selection_map;

	std::vector<std::string>                      m_counter_paths;
	std::unordered_map<std::string,node<counter>> m_counter_map;

};

template <typename T>
template <typename U>
class analysis<T>::node : public concurrent<U>
{

public:
	using node_type = typename concurrent<U>::node_type;

public:
	friend class analysis<T>;
	template <typename> friend class node;

public:
	node() :
		m_analysis(nullptr)
	{}
	node(analysis<T>& analysis, const concurrent<U>& action) :
		concurrent<U>(action),
		m_analysis(&analysis)
	{}
	~node() = default;

	template <typename V>
	node(const node<V>& other) :
		concurrent<U>(other),
		m_analysis(other.m_analysis)
	{}

	template <typename V>
	node& operator=(const node<V>& other)
	{
		m_analysis = other.m_analysis;
		this->m_islots.clear();
		for (size_t i=0 ; i<other.concurrency() ; ++i) {
			this->m_islots.push_back(other.slot(i));
		}
		return *this;
	}

	std::string name() const
	{
		return this->check( [] (const action& act) { return act.name(); } );
	}

	std::string path() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return this->check( [] (const selection& sel) { return sel.path(); } );
		} else if constexpr(std::is_base_of_v<counter,U>) {
			return this->check( [] (const counter& cnt) { return cnt.path(); } );
		} else {
			static_assert((std::is_base_of_v<selection,U> || std::is_base_of_v<counter,U>), "non-selection/counter node has no path");
		}
	}

	std::string full_path() const
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return this->check( [] (const selection& sel) { return sel.full_path(); } );
		} else if constexpr(std::is_base_of_v<counter,U>) {
			return this->check( [] (const counter& cnt) { return cnt.full_path(); } );
		} else {
			static_assert((std::is_base_of_v<selection,U> || std::is_base_of_v<counter,U>), "non-selection/counter node has no path");
		}
	}

  template <typename V = U, typename... Vars>
	typename std::enable_if<std::tuple_size<decltype(std::declval<V>().get_arguments())>::value!=0, void>::type evaluate(const node<Vars>&... arguments)
	{
		this->apply( [] (U& defn, Vars&... args) { defn.set_arguments(args...); }, arguments... );
	}

  template <typename Sel, typename F, typename... Vars>
  node<selection> filter(const std::string& name, F callable, const node<Vars>&... columns)
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_analysis->from(*this).template filter<Sel>(name, callable, columns...);
		} else {
			static_assert(std::is_base_of_v<selection,U>, "non-selection cannot chain selections");
		}
	}

  template <typename Sel, typename F, typename... Vars>
  node<selection> channel(const std::string& name, F callable, const node<Vars>&... columns)
	{
		if constexpr(std::is_base_of_v<selection,U>) {
			return m_analysis->from(*this).template channel<Sel>(name, callable, columns...);
		} else {
			static_assert(std::is_base_of_v<selection,U>, "non-selection cannot chain selections");
		}
	}

	void weighted(bool weighted=true)
	{
		if constexpr(std::is_base_of_v<counter,U>) {
			this->apply( [=] (counter& cnt) { cnt.use_weight(weighted); } );
		} else {
			static_assert(std::is_base_of_v<counter,U>, "non-counter cannot be set to be raw/weighted");
		}
	}

	void scale(double scale)
	{
		if constexpr(std::is_base_of_v<counter,U>) {
			this->apply( [=] (counter& cnt) { cnt.set_scale(scale); } );
		} else {
			static_assert(std::is_base_of_v<counter,U>, "non-counter cannot be scaled");
		}
	}

	template <typename... Cols>
	void fill(const node<Cols>&... columns)
	{
		if constexpr( is_counter_bookkeeper_v<U> ) {
			this->apply( [] (U& bookkeeper, Cols&... cols) { bookkeeper.enter_fill(cols...); }, columns... );
		} else if constexpr( is_counter_fillable_v<U> ) {
			this->apply( [] (U& fillable, Cols&... cols) { fillable.enter_fill(cols...); }, columns... );
		} else {
			static_assert( (is_counter_bookkeeper_v<U> ||  is_counter_fillable_v<U>), "non-fillable counter/bookkeeper" );
		}
	}

	template <typename Cnt, typename V = U, std::enable_if_t<std::is_base_of_v<selection,V>>* = nullptr>
	node<Cnt> book(const node<counter::bookkeeper<Cnt>>& bookkeeper)
	{
		return m_analysis->from(*this).template book(bookkeeper);
	}

	template <typename V = U, typename std::enable_if<is_counter_bookkeeper_v<V>,void>::type* = nullptr>
	node<typename V::implementation_type> book(const node<selection>& filter)
	{
		return m_analysis->from(filter).template book(*this);
	}

	template <typename V = U, std::enable_if_t<std::is_base_of_v<counter::implementation<V>,V>>* = nullptr>
	decltype(auto) result()
	{
		m_analysis->analyze();
		return this->model()->get_result();
	}

	template <typename V = U, typename std::enable_if<is_counter_bookkeeper_v<V>,void>::type* = nullptr>
	node<typename V::implementation_type> operator[](const std::string& sel_path)
	{
		return node<typename V::implementation_type>(*this->m_analysis, this->invoke([=](U& bookkeeper){ return bookkeeper.get_counter(sel_path); }) );
	}

protected:
	void merge_results()
	{
		if constexpr(std::is_base_of_v<counter,U>) {
			for (size_t islot=1 ; islot<this->concurrency() ; ++islot) {
				this->model()->merge_result(*this->slot(islot));
			}
		} else {
			static_assert(std::is_base_of_v<counter,U>,"non-counter cannot be merged");
		}
	}

protected:
	analysis<T>*  m_analysis;

};

}

// ----------------------------------------------------------------------------
// analysis
// ----------------------------------------------------------------------------

template <typename T>
ana::analysis<T>::analysis(std::unique_ptr<T> dataset) :
	sample<T>(std::move(dataset)),
	m_analyzed(false)
{}

template <typename T>
template <typename... Args>
ana::analysis<T>::analysis(Args&&... args) :
	sample<T>(std::forward<Args>(args)...),
	m_analyzed(false)
{}

template <typename T>
template <typename Val, typename... Args>
typename ana::analysis<T>::template node<ana::column<Val>> ana::analysis<T>::read(const std::string& name, const Args&... args)
{
	auto nd = node<column<Val>>(*this, this->m_processors.invoke( [=](table::processor<reader_type>& proc) { return proc.template read<Val>(name,args...); } ));
	this->add_term(nd);
	return nd;
}

template <typename T>
template <typename Val>
typename ana::analysis<T>::template node<ana::column<Val>> ana::analysis<T>::constant(const std::string& name, const Val& val)
{
	auto nd = node<column<Val>>(*this, this->m_processors.invoke( [=](table::processor<reader_type>& proc) { return proc.template constant<Val>(name,val); } ));
	this->add_term(nd);
  return nd;
}

template <typename T>
template <typename Def, typename... Args>
typename ana::analysis<T>::template node<Def> ana::analysis<T>::define(const std::string& name, const Args&... arguments)
{
	auto nd = node<Def>(*this, this->m_processors.invoke( [&](table::processor<reader_type>& proc) { return proc.template define<Def>(name,arguments...); } ));
	this->add_term(nd);
	return nd;
}

template <typename T>
template <typename F, typename... Vars>
auto ana::analysis<T>::evaluate(const std::string& name, F callable, const node<Vars>&... columns) ->  typename analysis<T>::template node<column<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>
{
	auto nd = node<column<std::decay_t<typename decltype(std::function(std::declval<F>()))::result_type>>>(*this, this->m_processors.invoke( [=](table::processor<reader_type>& proc, Vars&... vars) { return proc.template evaluate(name,callable,vars...); }, columns... ));
	this->add_term(nd);
  return nd;
}

template <typename T>
template <typename Sel, typename F, typename... Vars>
typename ana::analysis<T>::template node<ana::selection> ana::analysis<T>::filter(const std::string& name, F callable, const node<Vars>&... columns)
{
	auto nd = node<selection>(*this, this->m_processors.invoke( [=](table::processor<reader_type>& proc, Vars&... vars) { return proc.template filter<Sel>(name,callable,vars...); }, columns... ));
	this->add_selection(nd);
  return nd;
}

template <typename T>
template <typename Sel, typename F, typename... Vars>
typename ana::analysis<T>::template node<ana::selection> ana::analysis<T>::channel(const std::string& name, F callable, const node<Vars>&... columns)
{
	auto nd = node<selection>(*this, this->m_processors.invoke( [=](table::processor<reader_type>& proc, Vars&... vars) { return proc.template channel<Sel>(name,callable,vars...); }, columns... ));
	this->add_selection(nd);
  return nd;
}

template <typename T>
template <typename Cnt, typename... Args>
typename ana::analysis<T>::template node<ana::counter::bookkeeper<Cnt>> ana::analysis<T>::count(const std::string& name, const Args&... arguments)
{
	auto nd = node<counter::bookkeeper<Cnt>>(*this, this->m_processors.invoke( [=](table::processor<reader_type>& proc) { return proc.template count<Cnt>(name,arguments...); } ));
  return nd;
}

template <typename T>
template <typename Cnt>
typename ana::analysis<T>::template node<Cnt> ana::analysis<T>::book(const node<counter::bookkeeper<Cnt>>& bookkeeper)
{
	this->reset();
	auto nd = node<Cnt>(*this, this->m_processors.invoke( [=](table::processor<reader_type>& proc, const counter::bookkeeper<Cnt>& dlyd) { return proc.template book<Cnt>(dlyd); }, bookkeeper ));
	this->add_counter(nd);
  return nd;
}

template <typename T>
typename ana::analysis<T>::template node<ana::selection> ana::analysis<T>::operator[](const std::string& path) const
{
	if (!this->has_selection(path)) {
		throw std::logic_error("selection does not exist");
	};
	return m_selection_map.at(path);
}

template <typename T>
void ana::analysis<T>::analyze()
{ 
	if (!m_analyzed) {
		this->run_processors();
		this->merge_results();
		this->clear_counters();
	}
	m_analyzed = true;
}

template <typename T>
void ana::analysis<T>::reset()
{ 
	m_analyzed = false;
}

template <typename T>
void ana::analysis<T>::merge_results()
{ 
	for (auto& counter : m_counter_map) {
		counter.second.merge_results();
	}
}

template <typename T>
void ana::analysis<T>::clear_counters()
{ 
	m_counter_paths.clear();
	m_counter_map.clear();
	this->m_processors.apply( [] (table::processor<reader_type>& proc) { proc.clear_counters(); } );
}

template <typename T>
void ana::analysis<T>::run_processors()
{
	// start
	this->m_dataset->start();

	// multi-threaded
	if (multithread::status()) {
		// start threads
		std::vector<std::thread> pool;
		for (size_t islot=0 ; islot<this->m_processors.concurrency() ; ++islot) {
			pool.emplace_back(
				[] (table::processor<reader_type>& proc) {
					proc.process();
				},
				std::ref(*this->m_processors.slot(islot))
			);
		}
		// join threads
		for (auto&& thread : pool) {
			thread.join();
		}
	// single-threaded
	} else {
		for (size_t islot=0 ; islot<this->m_processors.concurrency() ; ++islot) {
			this->m_processors.slot(islot)->process();
		}
	}

	// finish
	this->m_dataset->finish();

}

template <typename T>
ana::analysis<T>& ana::analysis<T>::from(const node<selection>& rebase)
{
	this->m_processors.apply( [] (table::processor<reader_type>& proc, selection& sel) { proc.from(sel); }, rebase );	
  return *this;
}

template <typename T>
bool ana::analysis<T>::has_term(const std::string& name) const
{
	return m_column_map.find(name)!=m_column_map.end();
}

template <typename T>
bool ana::analysis<T>::has_selection(const std::string& path) const
{
	return m_selection_map.find(path)!=m_selection_map.end();
}

template <typename T>
bool ana::analysis<T>::has_counter(const std::string& name) const
{
	return m_counter_map.find(name)!=m_counter_map.end();
}

template <typename T>
void ana::analysis<T>::add_term(typename ana::analysis<T>::template node<term> node)
{
	auto name = node.name();
	if (this->has_term(name)) {
		throw std::logic_error("column already exists");
	}
	m_column_map[name] = node;
	m_column_names.push_back(name);
}

template <typename T>
void ana::analysis<T>::add_selection(typename ana::analysis<T>::template node<selection> node)
{
	auto path = node.path();
	if (this->has_selection(path)) {
		throw std::logic_error("selection already exists");
	}
	m_selection_map[path] = node;
	m_selection_paths.push_back(path);
}

template <typename T>
void ana::analysis<T>::add_counter(typename ana::analysis<T>::template node<counter> node)
{
	auto path = node.path();
	if (this->has_counter(path)) {
		throw std::logic_error("counter already exists");
	}
	m_counter_map[path] = node;
	m_counter_paths.push_back(path);
}

template <typename T>
std::vector<std::string> ana::analysis<T>::list_term_names() const
{
	return m_column_names;
}

template <typename T>
std::vector<std::string> ana::analysis<T>::list_selection_paths() const
{
	return m_selection_paths;
}

template <typename T>
std::vector<std::string> ana::analysis<T>::list_counter_paths() const
{
	return m_counter_paths;
}