Status: Active
Source Idea Path: ideas/open/392_rv64_va_list_expression_call_argument_value_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Route Or Repair The Owned Boundary

# Current Packet

## Just Finished

Post-repair Step 3 repaired the caller-side materialization boundary for
`load_local %lv.state.8` after `llvm.va_start`.

RV64 object emission now routes a `LoadLocalInst` of a va_start destination
address value to the active helper-owned destination stack slot when there is a
complete prepared RV64 variadic entry helper contract and a preceding same-block
`VaStart` helper operand home for that destination. This keeps the explicit
call-argument publication payload value intact, but materializes the payload
from the va_start-published word instead of the stale ordinary frame slot.

The focused backend fixture covers the representative shape by giving the load
a stale normal frame-slot memory access and a valid va_start helper destination
slot. The test asserts that the post-va_start load reads the helper-published
word (`ld a2,72(sp)` in the fixture), not the stale slot. Existing
call-argument value-publication fail-closed variants remain covered by the
backend suite.

Additional representative evidence shows the RV64 object route for
`va-arg-13.c` now passes:
`build/agent_state/392_postrepair_step3_va-arg-13.run.log` and
`build/agent_state/392_postrepair_step3_va-arg-13.case.log`.

## Suggested Next

The Step 3 slice is ready for supervisor review/commit. A useful next packet is
acceptance-oriented evidence or plan-owner closure/rewrite depending on whether
idea 392 is now complete beyond the representative route.

## Watchouts

- The helper-destination route is intentionally guarded by explicit prepared
  RV64 va_start helper operand homes; it does not infer authority from generic
  store publications.
- The repair keeps the caller argument object address route separate from the
  word stored into that object.
- The broader clang/RV64 scalar-`va_list` ABI difference remains a watchout,
  but the same-C4C representative object route now passes.

## Proof

Delegated proof command run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed
out of 326`.

Representative evidence run:
`build/agent_state/392_postrepair_step3_va-arg-13.run.log`.

Result: passed; `case_exit=0` and
`[PASS][rv64-gcc-torture-backend-obj] /workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c`.
