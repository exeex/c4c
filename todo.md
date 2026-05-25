Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 5 completed closure review for the indirect byval ABI size fallback
removal checkpoint.

Completed work:

- Rechecked surviving `calls*.cpp` and `calls*.hpp` retained
  `CallInst::arg_abi` / `CallInst::arg_types` reads against the source idea.
- Rejected closure because durable target-local authority leaks remain in
  local aggregate address publication, argument-source local frame address
  publication, indirect-register byval shape selection, and
  `calls_dispatch_bridge.hpp` `CallInst`-shaped helper boundaries.
- Confirmed `aarch64_indirect_byval_argument_size_bytes` no longer survives;
  the remaining indirect byval blocker is
  `aarch64_indirect_register_byval_argument`.

## Suggested Next

Keep `ideas/open/02_aarch64_calls_emission_consolidation.md` active. Step 1
should select the next retained metadata authority leak and record one focused
executor packet with its prepared replacement fact or missing prepared-fact
blocker.

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  durable blockers remain.
- The stack byval fallback path is no longer a blocker; do not re-add
  `aarch64_stack_byval_argument_size_bytes`.
- The removed indirect byval size fallback path is no longer a blocker; do not
  re-add `aarch64_indirect_byval_argument_size_bytes`.
- `aarch64_indirect_register_byval_argument` remains a retained
  `CallInst::arg_abi` shape predicate and is a strong next candidate.
- The publication blockers in `calls_dispatch_bridge.cpp` and
  `calls_argument_sources.cpp` remain separate durable candidates.
- Treat retained `CallInst` reads as acceptable only for identity validation,
  diagnostics, or emission context, not for rederiving prepared call-plan
  decisions.
- The indirect byval fallback intentionally remains separate from
  `prepared_byval_lane_extent_bytes`, which is only for small
  `"call_arg_byval_aggregate_register_lanes"` payload-lane moves.

## Proof

Closure review evidence:

- `rg -n "arg_abi|arg_types|CallInst" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls*.hpp src/backend/mir/aarch64/codegen/calls.hpp`
- `rg -n "aarch64_indirect_byval_argument_size_bytes|prepared_indirect_byval_extent_bytes|aarch64_indirect_register_byval_argument" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls*.hpp src/backend/mir/aarch64/codegen/calls.hpp`

Result: close rejected. Existing broad backend proof remains accepted from
`test_before.log`, which reports 162/162 `^backend_` tests passing. No
close-time regression guard was run because source-idea completion is false.
