Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended the Step 4 allocator so regalloc now publishes non-phi,
consumption-keyed `move_resolution` records for binary/select/cast consumers
when a named source value and the produced result land in different assigned
storage, reusing the existing `(from_value_id, to_value_id, block_index,
instruction_index)` contract instead of inventing a new surface mid-packet.

Focused `backend_prepare_liveness` coverage now proves that a call-crossing
value reused after the call can publish a non-phi transfer record: the
`weighted_post_call_pressure` shape keeps `carry` in callee-saved `s1`, assigns
the post-call `hot` result to caller-saved `t0`, and therefore records a
`consumer_register_to_register` move at the consuming instruction site.

## Suggested Next

Extend Step 4 move planning from result-producing non-phi consumers into
call/result and stack-backed late-use surfaces so regalloc can publish
consumption-keyed transfer records when a named value must be materialized for
call arguments or other non-result consumer sites, not only for phi joins and
result-producing instructions.

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
- `move_resolution` now covers phi joins plus result-producing
  binary/select/cast consumers, but it still does not model call-argument
  materialization, return-site transfers, or non-result consumer sites
- consumer-keyed move records intentionally skip redundant entries when the
  source and produced result already share the same assigned register or stack
  slot, and the current dedupe key is `(from_value_id, to_value_id,
  block_index, instruction_index)`
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
after teaching `run_regalloc()` to publish non-phi, consumer-keyed
move-resolution bookkeeping for result-producing instructions whose named
operands and produced values land in different assigned storage.
Canonical proof log: `test_after.log`.
