Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Extended the bounded Step 4 allocator into distinct semantic phases closer to
the retained Rust route: call-crossing values now consume a callee-saved pool
first, non-call-crossing values still prefer the caller-saved seed pool, and
caller-pool overflow can spill over into remaining callee-saved capacity
before falling back to real stack slots.

Focused `backend_prepare_liveness` coverage now proves that split directly on
a call-heavy function: the call-spanning `carry` value takes `s1`, the first
post-call value takes caller-saved `t0`, and the overlapping overflow value
takes the remaining callee-saved `s2` while the published allocation
constraints record the same preference split.

## Suggested Next

Extend Step 4 by moving beyond fixed pool ordering into broader Rust-like
priority handling: use liveness-derived weights to choose among multiple
eligible caller/callee candidates when the current bounded pools overflow, and
prove that higher-value intervals win before lower-priority ones fall back to
stack slots.

## Watchouts

- exact `definition_point` / `use_points` now capture true local activity,
  while `live_interval` still represents the CFG-extended range after
  live-through propagation; downstream consumers must not collapse those into
  one concept
- the current call-crossing split is still intentionally bounded: it proves
  phase ordering and spillover, but it does not yet choose among competing
  same-bank intervals with Rust-like weight-sensitive heuristics
- `PreparedAllocationConstraint.preferred_register_names` now carries the
  caller-vs-callee preference split, while `cannot_cross_call` is reserved for
  the stronger call-spanning prohibition; downstream consumers should keep
  those meanings distinct
- the active target pools are still small proof-oriented seeds, even though the
  callee-saved side now has bounded spillover headroom
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
after teaching `run_regalloc()` to split call-crossing values into a distinct
callee-saved phase and to use remaining callee-saved capacity as bounded
spillover for caller-pool overflow.
Canonical proof log: `test_after.log`.
