Status: Active
Source Idea Path: ideas/open/prealloc-regalloc-coordinator-contraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Contract A Second Stable Boundary

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: audited for one remaining stable regalloc
coordinator boundary after assignment expiry and stack-slot publication were
moved.

Decision: no code relocation in this packet. The remaining coordinator-local
helpers are not a clean second helper-family boundary:
- `assign_from_pool` and allocation fallback still coordinate mutable register
  pools, spill points, eviction state, and stack-slot seeds in one allocation
  pass, so moving them would be a broad allocation-order change risk.
- `append_prepared_call_abi_bindings(...)` and
  `append_prepared_abi_binding(...)` publish value-location ABI bundles, but
  they cross call-return ABI facts, regalloc value lookup, prepared move bundle
  grouping, and public dump meaning. Moving them now would belong with the
  Step 4 printer/public-contract audit rather than this helper-family packet.
- `build_prepared_value_location_function(...)` is coordinator glue that joins
  value homes, move-resolution records, and call ABI bindings; no existing
  `regalloc/` helper file owns that full contract without mixing unrelated
  families.

This keeps Step 3 behavior-preserving by leaving implementation files
unchanged rather than extracting an unstable mixed-boundary helper.

## Suggested Next

Move to Step 4 with a printer/public-contract audit. Check whether the current
prepared-printer output and `regalloc.hpp` aggregate contract still match the
new assignment and stack-slot helper boundaries; make only mirror edits if a
printed meaning or public name actually changed.

## Watchouts

`regalloc.cpp` still owns allocation phase order, mutable pool selection,
spill-point timing, value-location bundle assembly, and the final prepared
stack-layout frame-size/frame-alignment maxima. Avoid moving call ABI binding
or prepared value-location bundle helpers before auditing the public dump
contract, because that code affects printer-visible meaning even if it is
mechanically local.

## Proof

No code changes were made, per the delegated no-code path.

Ran `git diff --check`; passed.

Proof log: not produced for this no-code audit.
