Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 5 closure review rejected closure for the stack byval ABI size fallback
removal checkpoint.

Surviving durable authority leaks remain in `calls*.cpp`/`calls*.hpp`:

- `calls_dispatch_bridge.cpp` still decides local aggregate address
  publication eligibility from retained `CallInst::arg_abi` and
  `CallInst::arg_types`.
- `calls_argument_sources.cpp` still decides local frame address publication
  from retained `CallInst::arg_types` and `CallInst::arg_abi`.
- `calls_byval_aggregates.cpp` still decides indirect byval size and indirect
  register byval shape from retained `CallInst::arg_abi`.
- `calls_dispatch_bridge.hpp` still has `CallInst`-shaped helper boundaries
  that need retirement or emission-only justification after publication no
  longer reconstructs call-plan decisions.

## Suggested Next

Step 1 should select one of the remaining retained metadata authority leaks
for the next checkpoint, map it to an existing prepared fact or record the
missing prepared authority, and choose the focused proof command.

## Watchouts

- Do not close `ideas/open/02_aarch64_calls_emission_consolidation.md` while
  these durable blockers remain.
- The stack byval fallback path is no longer a blocker; do not re-add
  `aarch64_stack_byval_argument_size_bytes`.
- Treat retained `CallInst` reads as acceptable only for identity validation,
  diagnostics, or emission context, not for rederiving prepared call-plan
  decisions.

## Proof

Closure review evidence:
`rg -n "arg_abi|arg_types|CallInst" src/backend/mir/aarch64/codegen/calls*.cpp src/backend/mir/aarch64/codegen/calls*.hpp src/backend/mir/aarch64/codegen/calls.hpp`

Result: closure rejected because surviving retained `arg_abi`/`arg_types`
decision reads remain in dispatch publication, argument-source publication,
and indirect byval helper paths.

Latest accepted broad backend proof remains in `test_before.log`: `^backend_`
passed `162/162`.
