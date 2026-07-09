Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Repair The Narrow Stack-Home Consumer Route

# Current Packet

## Just Finished

Step 7 repaired the verified narrow RV64 stack-home consumer route:
- Routed floating byval stack-home loads through
  `prepared_byval_stack_slot_pointer_access_offset(...)` before the generic
  pointer-value materializer.
- Routed floating sret stack-home stores through
  `prepared_sret_stack_slot_pointer_access(...)` before the generic
  pointer-value materializer.
- Updated the floating local-memory diagnostic predicate to accept the same
  explicit byval/sret stack-home authority routes.
- Relaxed byval/sret helper-local `align_bytes > size_bytes` rejection only
  for explicit stack-home authority, while still requiring RV64 width,
  nonzero/at-most-8 access alignment, home/object alignment compatibility,
  selected in-bounds range, frame bounds, and signed-12-bit encodability.
- Added RV64 object-emission coverage for the F32 byval load route, F32 sret
  store route, malformed authority negatives, and a byte-slice byval
  stronger-alignment cross-check.

## Suggested Next

Step 8 should rerun the aggregate stack-home residual probe used by Step 5 and
classify whether the clean byval/sret representatives move past the generic
local-memory gate after the Step 7 RV64 consumer repair.

## Watchouts

The generic pointer-value stack-home helper still excludes `byval_param` and
`sret_param`; Step 7 kept that boundary and uses explicit stack-home authority
instead.

Keep F128/16-byte local-memory rows, move-bundle rows, mixed local/global rows,
runtime mismatches, and unrelated aggregate copies out of the next
reclassification packet.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: passed, 1/1.

Extra check: `git diff --check` passed.

Proof log: `test_after.log`.
