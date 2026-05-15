Status: Active
Source Idea Path: ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Backend Validation And Closure Readiness

# Current Packet

## Just Finished

Plan Step 5 - Broader Backend Validation And Closure Readiness completed for
the active DMB carrier packet.

The DMB route is closure-ready within the packet boundary:

- Semantic BIR now records the no-result AArch64 `BarrierDmb` carrier with its
  immediate operand authority.
- Prepared carrier support preserves the DMB family, feature, operand role,
  and immediate value without inventing a result home or operand value home.
- Dispatch and machine-printer boundary tests prove complete DMB carriers
  remain non-selected and cannot leak through scalar, CRC, vector, call, or
  printer-selected paths.
- Broader backend validation passed after these carrier and boundary updates.

## Suggested Next

Ask the plan owner for a lifecycle decision on the active
barrier/cache/hint/builtin carrier plan: close the completed DMB packet, or
split any remaining non-DMB families into follow-up source intent.

## Watchouts

- DMB selected-machine support remains absent by design. This route completed
  semantic/prepared carrier authority plus closed machine boundaries, not
  selected `dmb` lowering or printer spelling.
- Cache-maintenance, pause/hint, and builtin-address representatives remain
  outside this DMB packet. Builtin-address still overlaps separate
  address-materialization/TLS authority and should not be silently folded into
  this completed DMB carrier closure.

## Proof

Delegated proof for the Step 5 broader backend validation and closure-readiness
packet:

- `set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`
- Result: passed, 139/139 backend tests green after an up-to-date build.
- Proof log: `test_after.log`
