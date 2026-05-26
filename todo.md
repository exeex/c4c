Status: Active
Source Idea Path: ideas/open/17_aarch64_absent_selection_fallback_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Retire Indirect Byval Lane Payload Fallbacks

# Current Packet

## Just Finished

Step 3: Retire Indirect Byval Lane Payload Fallbacks is complete.
`calls_moves.cpp` now treats `ByvalRegisterLane` selected-source facts as the
only byval lane extent authority and requires
`make_byval_register_lane_prepared_source` to produce complete prepared source
bytes for register-lane, register-home, indirect byval, and stack-lane
publication. The old BIR-store reconstruction route was removed:
`collect_byval_register_lane_stores`, `aggregate_lane_store_memory`, fragmented
byval publication helpers, payload-store matching helpers, and the indirect
byval absent-selection extent fallback are no longer declared or used.
`backend_aarch64_instruction_dispatch_test.cpp` and
`backend_aarch64_call_boundary_owner_test.cpp` now cover positive prepared
`ByvalRegisterLane` register and stack publication, plus absent/incomplete
selected-source fail-closed cases.
Follow-up `00204` validation found that prepared byval selections were still
using preservation homes for some small aggregate payloads. `call_plans.cpp`
now publishes the selected source from complete prepared payload facts: first
from contiguous prepared aggregate load-source accesses for assembled register
lanes, then from contiguous local aggregate payload stores. This fixes the
AArch64 byval/string corruption without restoring target-local BIR-store
reconstruction as source authority.

## Suggested Next

Supervisor should review the Step 3 slice for route quality and decide whether
the active plan needs a plan-owner close/deactivate/split decision or another
bounded packet from the remaining runbook.

## Watchouts

- Do not edit the source idea for routine audit findings.
- Do not treat expectation downgrades or fallback renames as progress.
- Keep `review/` artifacts transient.
- `clang-format` is not installed in this environment; formatting was kept
  manual.
- `calls_byval_aggregates.cpp` is now intentionally small; remaining byval
  authority is the selected-source adapter, not BIR payload-store analysis.

## Proof

Ran the delegated proof:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|c_testsuite_aarch64_backend_src_00204_c)$"'`.
Result: passed. Proof log: `test_after.log`.
Also ran `git diff --check`; result: clean.
