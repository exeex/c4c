Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Local Decision

# Current Packet

## Just Finished

Step 2 removed the selected retained AArch64 byval metadata authority leak from
the aggregate stack call-argument copy path.

Completed work:

- `calls_moves.cpp` no longer calls
  `aarch64_stack_byval_argument_size_bytes`; the selected path now takes the
  copy extent from `PreparedValueHome::size_bytes` or, when absent, the
  prepared `PreparedCallArgumentPlan::destination_stack_size_bytes`.
- Deleted the now-dead `aarch64_stack_byval_argument_size_bytes` declaration
  from `calls.hpp`.
- Deleted the helper definition from `calls_byval_aggregates.cpp`, removing its
  retained `CallInst::arg_abi[argument.arg_index]` byval-shape and size reads.

## Suggested Next

Step 3 helper-boundary closeout for the affected selected path: confirm there
is no surviving obsolete `aarch64_stack_byval_argument_size_bytes` API boundary
or include/build fallout, then let the supervisor decide whether to advance to
the broader backend checkpoint.

## Watchouts

- The selected path now prefers the source home size to preserve the aggregate
  copy extent and falls back only to prepared destination stack size. It does
  not rederive byval shape or size from retained `CallInst::arg_abi`.
- This packet intentionally did not widen into
  `aarch64_indirect_byval_argument_size_bytes`,
  `aarch64_indirect_register_byval_argument`, or local address publication
  helpers.

## Proof

Step 2 focused proof:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner)$') > test_after.log 2>&1`

Result: passed, `2/2` focused tests in `test_after.log`.
