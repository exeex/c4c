Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 by activating one bounded Rust-like copy-coalescing gain in
the C++ stack-layout path: lowering-scratch objects already identified as safe
single-store/single-load copy-coalescing candidates now skip dedicated frame
slot assignment, and `backend_prepare_stack_layout` proves that this changes
prepared output for a real function shape alongside dead-slot elision.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the C++ alloca escape/use-block
logic to `alloca_coalescing.rs` and activate the next bounded non-copy
coalescing gain, preferably by tightening which local-slot address and
cross-block uses still force permanent home-slot storage.

## Watchouts

- the active C++ route now elides dedicated frame slots for dead local-slot
  objects and for prepared `copy_coalescing_candidate` lowering-scratch objects
- `regalloc_helpers.cpp` still keeps most live local-slot objects conservative,
  so broader Rust-like alloca tiering is not active yet
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$'` successfully;
canonical proof log: `test_after.log`.
