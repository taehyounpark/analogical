#pragma once

#include <string>
#include <vector>
#include <memory>

#include "ana/counter.h"
#include "ana/cutflow.h"

namespace ana
{

template <typename Cnt> struct is_counter_booker: std::false_type {};
template <typename Cnt> struct is_counter_booker<counter::booker<Cnt>>: std::true_type {};
template <typename Cnt> constexpr bool is_counter_booker_v = is_counter_booker<Cnt>::value;

class counter::experiment : public selection::cutflow
{

public:
	experiment(double scale);
	~experiment() = default;

public:
	template <typename Cnt, typename... Args>
	std::shared_ptr<booker<Cnt>> book(Args&&... args);

	template <typename Cnt>
	auto book_selection(booker<Cnt> const& bkr, const selection& sel) -> std::shared_ptr<Cnt>;

	template <typename Cnt, typename... Sels>
	auto book_selections(booker<Cnt> const& bkr, Sels const&... sels) -> std::shared_ptr<booker<Cnt>>;

	void clear_counters();

protected:
	void add_counter(counter& cnt);

protected:
	std::vector<counter*> m_counters;
	const double m_norm;

};

}

template <typename Cnt, typename... Args>
std::shared_ptr<ana::counter::booker<Cnt>> ana::counter::experiment::book(Args&&... args)
{
	auto bkr = std::make_shared<booker<Cnt>>(std::forward<Args>(args)...);
	return bkr;
}

template <typename Cnt>
auto ana::counter::experiment::book_selection(booker<Cnt> const& bkr, const selection& sel) -> std::shared_ptr<Cnt>
{
	auto cnt = bkr.book_selection(sel);
	cnt->apply_scale(m_norm);
	this->add_counter(*cnt);
	return cnt;
}

template <typename Cnt, typename... Sels>
auto ana::counter::experiment::book_selections(booker<Cnt> const& bkr, Sels const&... sels) -> std::shared_ptr<booker<Cnt>>
{
	// get a booker that has all the selections added
	auto bkr2 = bkr.book_selections(sels...);
	// add all the counters (each with one selection) into the experiment
	for (auto const& sel_path : bkr2->list_selection_paths()) {
		auto cnt = bkr2->get_counter(sel_path);
		cnt->apply_scale(m_norm);
		this->add_counter(*cnt);
	}	
	return bkr2;
}