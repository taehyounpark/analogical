#pragma once

#include <vector>
#include <algorithm>
#include <type_traits>
#include <numeric>

namespace vec
{

template <typename T>
T sum(const std::vector<T>& vec);

template<typename T>
std::vector<std::vector<T>> group(const std::vector<T>& vec, size_t n);

}

template <typename T>
T vec::sum(const std::vector<T>& vec)
{
	return std::accumulate(std::next(vec.begin()), vec.end(), vec.front());
}

template<typename T>
std::vector<std::vector<T>> vec::group(const std::vector<T>& vec, size_t n)
{
	std::vector<std::vector<T>> grouped;
	size_t length = vec.size() / n;
	size_t remain = vec.size() % n;
	size_t begin = 0;
	size_t end = 0;
	for (size_t i = 0; i < std::min(n, vec.size()); ++i)
	{
		end += (remain > 0) ? (length + !!(remain--)) : length;
		grouped.push_back(std::vector<T>(vec.begin()+begin, vec.begin()+end));
		begin = end;
	}
	return grouped;
}