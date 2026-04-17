Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended the bounded Step 4 allocator so regalloc now publishes join-time
`move_resolution` entries when phi inputs and phi results land in different
storage, and spill bookkeeping emits reload operations for each later use after
an interval is evicted to a stack slot instead of only the first post-spill
use.

Focused `backend_prepare_liveness` coverage now proves both additions: a new
`phi_join_move_resolution` shape forces one phi incoming onto a stack slot
while the joined phi result remains register-backed and therefore publishes a
`phi_join_stack_to_register` move record, and the expanded
`evicted_value_spill_ops` shape records one spill plus a reload for each later
`local0` use after eviction.

## Suggested Next

Extend Step 4 from bounded phi/join bookkeeping into broader consumption-aware
move planning: when values stay register-backed across non-phi copies or
call-adjacent late uses, publish transfer records keyed to the consuming site
instead of only phi joins and fully evicted stack reloads.

## Watchouts

- exact `definition_point` / `use_points` now capture true local activity,
  while `live_interval` still represents the CFG-extended range after
  live-through propagation; downstream consumers must not collapse those into
  one concept
- loop-depth weighting now depends on mapping liveness use points back through
  `PreparedLivenessBlock` ranges; if program-point numbering changes, regalloc
  weighting and spill-op locations need to stay in lockstep with that contract
- the new spill bookkeeping only records real eviction-to-stack fallout for
  values that actually lose a register and stay stack-backed afterward; reloads
  are now emitted per later use point, but this is still bounded bookkeeping
  rather than true split-interval placement
- `move_resolution` is currently a phi-join storage-reconciliation surface: it
  compares the assigned storage of named phi incomings against the phi result
  and intentionally skips redundant entries when both sides already share the
  same register or stack slot
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
build -j --output-on-failure -R ^backend_prepare_liveness$ > test_after.log
2>&1`
after teaching `run_regalloc()` to publish phi-join move-resolution bookkeeping
for storage mismatches and to emit reload bookkeeping for each later use of an
evicted stack-backed value.
Canonical proof log: `test_after.log`.
