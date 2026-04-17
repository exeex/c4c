Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended the Step 4 allocator so named `CallInst.args` now publish call-site
`move_resolution` records when the consumed argument storage kind differs from
the assigned source storage, while reusing the existing
`(from_value_id, to_value_id, block_index, instruction_index)` surface by
self-keying non-result call-argument transfers to the consumed source value.

Focused `backend_prepare_liveness` coverage now proves the exact call-site
behavior: the new `call_arg_move_resolution` shape keeps `carry0`/`carry1` in
callee-saved `s1`/`s2`, assigns `keep.arg` to caller-saved `t0`, forces
`spill.arg` into a real stack slot under the same call-site pressure, and
therefore records a `call_arg_stack_to_register` move at the call instruction.

## Suggested Next

Extend Step 4 move planning from register-vs-stack call-argument materialization
into the remaining non-result consumer surfaces, starting with return-site
transfers or call-argument cases whose consumed location needs more precision
than the current register-vs-stack ABI kind split.

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
  binary/select/cast consumers plus call-site argument materialization, but it
  still does not model return-site transfers or other non-result consumer sites
- consumer-keyed move records intentionally skip redundant entries when the
  source and produced result already share the same assigned register or stack
  slot, and the current dedupe key is `(from_value_id, to_value_id,
  block_index, instruction_index)`
- call-argument move records currently compare assigned source storage against
  the ABI storage kind from `CallArgAbiInfo` and self-key the move record to
  the consumed source value because the current public contract does not expose
  a separate per-argument destination value or physical arg-register identity
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
after teaching `run_regalloc()` to publish call-site move-resolution
bookkeeping for named call arguments whose assigned storage and ABI-consumed
storage kind differ.
Canonical proof log: `test_after.log`.
