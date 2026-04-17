Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended the Step 4 allocator so named `ReturnTerminator` operands now publish
return-site `move_resolution` records when the returned value's assigned
storage differs from the function ABI-consumed return storage kind, using the
existing `(from_value_id, to_value_id, block_index, instruction_index)`
surface by self-keying the transfer to the returned value at the terminator
slot.

Focused `backend_prepare_liveness` coverage now proves both sides of the new
surface directly: `return_move_resolution` returns an `F32` value whose active
regalloc assignment falls back to a real stack slot while the function return
ABI still models a register return, and therefore records a
`return_stack_to_register` move at the return terminator; the sibling
`return_same_storage_resolution` shape returns an `I32` value that stays
register-backed and therefore emits no redundant self-keyed return move.

## Suggested Next

Expose a more precise destination surface for call-site and return-site moves
so Step 4 bookkeeping can distinguish concrete ABI destinations instead of only
storage-kind mismatches and self-keyed value ids.

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
  slot, and the current dedupe key is `(from_value_id, to_value_id,
  block_index, instruction_index)`
- call-argument move records currently compare assigned source storage against
  the ABI storage kind from `CallArgAbiInfo` and self-key the move record to
  the consumed source value because the current public contract does not expose
  a separate per-argument destination value or physical arg-register identity
- call-result move records currently compare assigned result storage against
  the ABI storage kind from `CallResultAbiInfo` and self-key the move record to
  the produced result value because the current public contract does not expose
  a separate physical return-register identity
- return-site move records currently compare assigned result storage against a
  coarse function-level return storage kind derived from `bir::Function`
  instead of an explicit per-target return-ABI record, so they intentionally
  prove storage-kind mismatches without yet naming a physical return register
- the focused call-result proof currently uses an `F32` value on the active
  RISC-V target because the general-register seed pools are active while float
  register pools are still absent; if float allocation becomes active later,
  this test shape and the new return-move proof will need a different pressure
  source
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
after teaching `run_regalloc()` to publish return-site move-resolution
bookkeeping for named return values whose assigned storage and ABI-consumed
storage kind differ.
Canonical proof log: `test_after.log`.
