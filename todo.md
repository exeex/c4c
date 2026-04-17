Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended the Step 4 public `PreparedMoveResolution` contract with explicit
destination metadata: `destination_kind`, `destination_storage_kind`, and an
optional `destination_abi_index`. `run_regalloc()` now populates that surface
for generic value-to-value moves plus ABI-targeted call-argument, call-result,
and return-site transfers, while preserving the existing `(from_value_id,
to_value_id, block_index, instruction_index)` shape and tightening dedupe so
ABI-indexed call-argument destinations do not collapse together.

Focused `backend_prepare_liveness` coverage now asserts both families: phi and
consumer moves still publish the generic `Value` destination surface, while
call-argument, call-result, and return-site bookkeeping now prove the concrete
ABI destination kind and storage kind, including the argument index for the
stack-backed fourth call argument.

## Suggested Next

Thread concrete physical destination names through the new move-resolution ABI
surface once the active BIR/prealloc path exposes per-target argument and
return-register identities, so Step 4 can distinguish ABI classes from actual
register targets.

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
  binary/select/cast consumers plus call-site argument/result materialization,
  plus function return-site transfers, but it still does not model other
  non-result consumer sites
- consumer-keyed move records intentionally skip redundant entries when the
  source and produced result already share the same assigned register or stack
  slot; the dedupe key now also includes ABI destination metadata so repeated
  self-keyed ABI moves can stay distinguishable by destination kind/index
- call-argument, call-result, and return-site move records now publish
  destination kind plus coarse destination storage, but the active public
  contract still does not carry concrete physical register names
- return-site move records still derive their destination storage from the
  coarse function-level return type on `bir::Function`; a later packet should
  switch that over to explicit per-target return ABI metadata once it exists
- the focused call-result proof currently uses an `F32` value on the active
  RISC-V target because the general-register seed pools are active while float
  register pools are still absent; if float allocation becomes active later,
  this test shape and the return/ABI destination proofs will need a different
  pressure source
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
after teaching `run_regalloc()` to publish explicit ABI destination metadata in
`move_resolution` records for call-site and return-site transfers.
Canonical proof log: `test_after.log`.
