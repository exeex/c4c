#pragma once

#include "module.hpp"

namespace c4c::backend::prepare {

void populate_call_plans(PreparedBirModule& prepared);

void seed_supported_prior_call_preservations_from_current_call(
    PreparedCallPlansFunction& function_plan,
    const PreparedCallPlan& current_call_plan);

}  // namespace c4c::backend::prepare
