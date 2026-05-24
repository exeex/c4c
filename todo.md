Status: Active
Source Idea Path: ideas/open/prealloc-call-plan-phase-split.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Argument Planning Subphases

# Current Packet

## Just Finished

Completed `plan.md` Step 2 cleanup: removed the redundant
`arg_index < call->args.size()` source-materialization guard left after the
argument destination extraction and normalized the block indentation.

Changed files:
- `src/backend/prealloc/call_plans.cpp`
- `todo.md`

Cleanup result:
- The source-materialization block in `populate_call_plans` now sits directly
  under the per-argument loop after assigning `arg_plan.destination_*`.
- Removed the redundant guard already guaranteed by
  `for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index)`.
- Preserved source materialization behavior, destination extraction behavior,
  result planning behavior, and boundary-effect code.
- Left `calls.hpp`, `call_plans.hpp`, tests, `plan.md`, and the source idea
  untouched.

Deferred source-materialization candidate:
- The source side of the per-argument loop remains broad and is still the next
  natural Step 2 extraction candidate. It owns named value homes, pointer-base
  IDs, symbol addresses, immediates, F128 constant regalloc lookup, and source
  register placement.

## Suggested Next

Continue `plan.md` Step 2 with a narrow file-local extraction of argument
source materialization inside `src/backend/prealloc/call_plans.cpp`, keeping
`calls.hpp` unchanged unless a compile error proves a direct need.

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

## Proof

Ran delegated proof:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed, 162/162 backend tests passed.

Proof log: `test_after.log`.
