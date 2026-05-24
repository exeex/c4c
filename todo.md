Status: Active
Source Idea Path: ideas/open/prealloc-call-plan-phase-split.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Argument Planning Subphases

# Current Packet

## Just Finished

Completed `plan.md` Step 2 cleanup: extracted argument source materialization
from the `populate_call_plans` per-argument loop into file-local
`plan_call_argument_source(...)`.

Changed files:
- `src/backend/prealloc/call_plans.cpp`
- `todo.md`

Extraction result:
- Added file-local `CallArgumentSourcePlan` with explicit source-side outputs
  for storage encoding, value IDs, pointer-base IDs, literals, symbol names,
  registers, slots, register banks, pointer deltas, and register placement.
- Moved named value-home lookup, pointer-base ID resolution, symbol address
  handling, immediate handling, F128 constant regalloc lookup, source value ID
  lookup, and source register placement into `plan_call_argument_source(...)`.
- Kept the shared direct/indirect argument path intact; the per-argument loop
  now composes destination planning and source planning through separate
  file-local helpers.
- Preserved destination extraction behavior, result planning behavior,
  memory-return behavior, boundary-effect code, and aggregate-subslot fallback
  behavior.
- Left `calls.hpp`, `call_plans.hpp`, tests, `plan.md`, and the source idea
  untouched.

## Suggested Next

Proceed to `plan.md` Step 3 with a narrow mutable-state audit of the call-plan
families, using the extracted destination/source helpers as the boundary for
checking whether remaining local state should stay in `populate_call_plans` or
move behind another helper.

## Watchouts

- Keep this slice behavior-preserving.
- Do not change ABI policy, argument/result placement, clobbering,
  preservation, indirect-callee handling, memory-return behavior, formal
  publication behavior, or prepared dump meaning.
- Keep `calls.hpp` as the aggregate public contract unless usage proves a
  smaller independently consumed boundary.
- Preserve the current stack-binding preference in `find_call_abi_binding`; the
  destination helper deliberately calls that function unchanged.
- Do not split by direct-vs-indirect argument planning; the argument path is
  shared.
- Keep memory-return and aggregate-subslot fallback behavior out of argument
  source-materialization cleanup.
- `plan_call_argument_source(...)` intentionally mutates `prepared.names` for
  link-name interning exactly where the old loop did.

## Proof

Ran delegated proof:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed, 162/162 backend tests passed.

Proof log: `test_after.log`.
