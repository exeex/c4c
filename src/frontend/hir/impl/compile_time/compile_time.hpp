#pragma once

// Private compile-time and materialization implementation index.
//
// Compile-time/materialization translation units include this subdomain header
// instead of reaching directly through app-facing public contracts. The public
// contract remains src/frontend/hir/compile_time_engine.hpp for pipeline and
// dump callers.

#include "../../compile_time_engine.hpp"

