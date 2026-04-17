Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended the bounded Step 4 allocator with weight-sensitive same-pool
replacement using the existing liveness-derived `priority` / `spill_weight`
data. When the current caller or spillover callee pools overflow, a stronger
interval can now evict a weaker in-pool assignment instead of spilling
immediately, while call-crossing callee assignments remain protected from
non-call spillover.

Focused `backend_prepare_liveness` coverage now proves that behavior on a
post-call pressure shape: the call-spanning `carry` value keeps `s1`, the
higher-priority `hot` interval evicts into caller-saved `t0`, the displaced
lower-priority `low0` interval falls back to `s2`, and the remaining
lower-priority `low1` interval falls back to a real stack slot.

## Suggested Next

Extend Step 4 from bounded replacement into more Rust-like eviction policy:
record explicit spill/reload consequences for displaced intervals and incorporate
loop-depth or call-pressure bias into the ranking so tied same-bank intervals
are no longer ordered only by the current generic priority fields.

## Watchouts

- exact `definition_point` / `use_points` now capture true local activity,
  while `live_interval` still represents the CFG-extended range after
  live-through propagation; downstream consumers must not collapse those into
  one concept
- the new bounded replacement policy is still seed-pool oriented: it chooses
  among current caller/callee seeds using existing `priority` / `spill_weight`,
  but it does not yet publish explicit spill/reload ops for displaced
  intervals or model full Rust-style tiering
- `PreparedAllocationConstraint.preferred_register_names` now carries the
  caller-vs-callee preference split, while `cannot_cross_call` is reserved for
  the stronger call-spanning prohibition; downstream consumers should keep
  those meanings distinct
- the active target pools are still small proof-oriented seeds, even though the
  callee-saved side now has bounded spillover headroom
- non-call spillover must not evict call-crossing callee assignments; the new
  replacement helper intentionally restricts callee spillover eviction to
  other non-call occupants
- Rust Tier 2 / Tier 3 shared-slot reuse is still reference-only for the
  active C++ route: `PreparedFrameSlot` remains a dedicated object-owned slot
  record, so do not fake value-level reuse with object names, source kinds, or
  cross-object slot alias shortcuts before a later packet consumes the new
  value-level contract semantically
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran the delegated proof command successfully:
`cmake --build --preset default --target c4c_backend -j4 && ctest --test-dir
build -j --output-on-failure -R ^backend_prepare_liveness$ >
test_after.log 2>&1`
after teaching `run_regalloc()` to use bounded weight-sensitive replacement
inside the current caller/callee split and adding focused post-call pressure
coverage for the new behavior.
Canonical proof log: `test_after.log`.
