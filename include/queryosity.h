#pragma once

/**
 * @defgroup ext Extensions
 * @defgroup api API
 * @defgroup abc ABCs
 */

#include "queryosity/multithread.h"

#include "queryosity/dataset_reader.h"

#include "queryosity/column_definition.h"
#include "queryosity/column_equation.h"
#include "queryosity/column_reader.h"
#include "queryosity/column_series.h"

#include "queryosity/selection_yield.h"

#include "queryosity/query_aggregation.h"
#include "queryosity/query_definition.h"
#include "queryosity/query_series.h"

#include "queryosity/dataflow.h"

#include "queryosity/lazy.h"
#include "queryosity/lazy_varied.h"
#include "queryosity/todo.h"
#include "queryosity/todo_varied.h"

namespace qty = queryosity;