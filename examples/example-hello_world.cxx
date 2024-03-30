#include "queryosity/json.h"
#include "queryosity/hist.h"

#include "queryosity.h"

#include <fstream>
#include <vector>
#include <sstream>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

int main()
{

	dataflow df(multithread::enable(10));

	std::ifstream data("data.json");
	auto [x, w] = df.read(
			dataset::input<json>(data),
			dataset::column<std::vector<double>>("x_nom"),
			dataset::column<double>("w_nom"));

	auto zero = df.define(column::constant(0));
	auto x0 = x[zero];

	auto sel = df.weight(w).filter(
			column::expression([](std::vector<double> const &v)
												 { return v.size(); }),
			x);

	auto h_x0_w = df.make(
											query::plan<h1d>(linax(100, 0.0, 1.0)))
										.fill(x0)
										.book(sel)
										.result();

	// std::ostringstream os; os << *h_x0_w;
	// std::cout << os.str() << std::endl;
}