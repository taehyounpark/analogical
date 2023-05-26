#include "ana/selection.h"
#include "ana/counter.h"

std::string ana::selection::concatenate_names(std::vector<std::string> const& names, std::string delimiter)
{
	std::string joined;	
	for (auto const& name : names) {
		joined += name;
		joined += delimiter;
	}
	return joined;
}

ana::selection::selection(const std::string& name, bool channel) :
	m_name(name),
	m_channel(channel),
	m_preselection(nullptr)
{}

void ana::selection::set_initial()
{
	m_preselection = nullptr;
}

void ana::selection::set_previous(const ana::selection& preselection)
{
	m_preselection = &preselection;
}

bool ana::selection::is_initial() const
{
	return m_preselection ? false : true;
}

const ana::selection* ana::selection::get_previous() const
{
	return m_preselection;
}

bool ana::selection::is_channel() const noexcept
{
	return m_channel;
}

std::string ana::selection::get_name() const
{
	return m_name;
}

std::string ana::selection::get_path() const
{
	std::vector<std::string> channels;
	const selection* presel = this->get_previous();
	while (presel) {
		if (presel->is_channel()) channels.push_back(presel->get_name());
		presel = presel->get_previous();
	}
	std::reverse(channels.begin(),channels.end());
	return concatenate_names(channels,"/")+this->get_name();
}

std::string ana::selection::get_full_path() const
{
	std::vector<std::string> presels;
	const selection* presel = this->get_previous();
	while (presel) {
		presels.push_back(presel->get_name());
		presel = presel->get_previous();
	}
	std::reverse(presels.begin(),presels.end());
	return concatenate_names(presels,"/")+this->get_name();
}

void ana::selection::initialize()
{
	m_decision->initialize();
}

void ana::selection::execute()
{
	m_decision->execute();
}

void ana::selection::finalize()
{
	m_decision->finalize();
}