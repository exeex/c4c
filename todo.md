Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by extending the C++ alloca/use bridge for
call-escaped local-slot addresses: `alloca_coalescing.cpp` now treats
pointer-typed call arguments whose value names resolve to local-slot roots as
real addressed uses/escapes, so those slots no longer get misclassified as
dead and elided from frame-slot assignment. `backend_prepare_stack_layout` now
proves this with a focused call-escaped local-slot activation case alongside
the earlier dead-slot, copy-coalescing, and addressed-local-slot coverage.

## Suggested Next

Continue Step 2 in `stack_layout`: keep comparing the C++ alloca/use-block
bridge to `alloca_coalescing.rs` and activate the next bounded gain around
remaining pointer-derived escape/use sites that still are not modeled, with
inline-asm pointer operands and other non-load/store pointer consumers as the
next likely bounded candidates.

## Watchouts

- the active C++ route now elides dedicated frame slots for dead local-slot
  objects and for prepared `copy_coalescing_candidate` lowering-scratch objects,
  while both addressed-local-slot roots and call-escaped local-slot roots now
  correctly stay live and address-exposed
- `regalloc_helpers.cpp` still keeps most live local-slot objects conservative,
  so broader Rust-like alloca tiering is not active yet
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
successfully; canonical proof log: `test_after.log`.
