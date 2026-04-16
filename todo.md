Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 by tightening the C++ alloca/home-slot bridge against
`alloca_coalescing.rs`: stack-layout now treats `MemoryAddress::LocalSlot`
base-slot references as real addressed uses of the root local slot, so
addressed local storage no longer gets misclassified as dead and elided from
frame-slot assignment. `backend_prepare_stack_layout` now proves this with a
cross-block addressed-local-slot shape alongside the earlier dead-slot and
copy-coalescing coverage.

## Suggested Next

Continue Step 2 in `stack_layout`: keep comparing the C++ alloca/use-block
bridge to `alloca_coalescing.rs` and activate the next bounded gain around
remaining address-derived escape/use sites that the current C++ path still does
not model, especially beyond direct `MemoryAddress::LocalSlot` base-slot
references.

## Watchouts

- the active C++ route now elides dedicated frame slots for dead local-slot
  objects and for prepared `copy_coalescing_candidate` lowering-scratch objects,
  but addressed local-slot roots now correctly stay live and address-exposed
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

Attempted broader checkpoint with `ctest --test-dir build --output-on-failure
-L 'backend'`; it still fails in the existing `c_testsuite_x86_backend_*`
external tests due backend asm-path / fallback-IR errors unrelated to this
stack-layout packet.
