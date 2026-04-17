Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended the bounded Step 4 allocator so regalloc ranking now incorporates the
existing liveness loop-depth data instead of raw use counts alone, and explicit
spill bookkeeping is published when an evicted register value really falls back
to a stack slot.

Focused `backend_prepare_liveness` coverage now proves both additions: a
loop-carried `loop.hot` value publishes loop-depth-weighted priority /
spill-weight above its raw non-loop floor, and an `evicted_value_spill_ops`
shape records explicit `spill` / `reload` operations when a hotter post-call
interval takes the lone caller-saved seed while both callee-saved registers are
already protected by call-crossing values.

## Suggested Next

Extend Step 4 from bounded spill bookkeeping into more Rust-like move/reload
planning: when stack-backed values and register-backed values meet at joins or
late uses, publish `move_resolution` and more precise reload points instead of
only the first spill/reload markers for fully evicted intervals.

## Watchouts

- exact `definition_point` / `use_points` now capture true local activity,
  while `live_interval` still represents the CFG-extended range after
  live-through propagation; downstream consumers must not collapse those into
  one concept
- loop-depth weighting now depends on mapping liveness use points back through
  `PreparedLivenessBlock` ranges; if program-point numbering changes, regalloc
  weighting and spill-op locations need to stay in lockstep with that contract
- the new spill bookkeeping only records real eviction-to-stack fallout for
  values that actually lose a register and stay stack-backed afterward; it does
  not yet publish join moves or repeated reload/store traffic for split
  intervals
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
after teaching `run_regalloc()` to weight liveness uses by loop depth and to
publish explicit spill/reload bookkeeping for values that are evicted from a
register and finish stack-backed.
Canonical proof log: `test_after.log`.
