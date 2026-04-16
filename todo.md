Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by making single-input pointer transforms
explicit escape points in the active C++ alloca/use bridge. `alloca_coalescing.cpp`
now routes `CastInst` through a dedicated single-input pointer-transform helper
that conservatively marks rooted local-slot pointers as escaped while still
preserving alias propagation, and `backend_prepare_stack_layout` proves that a
cast-only pointer path keeps the root slot address-exposed and home-slotted
without relying on a later call/store escape site.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the remaining C++ pointer-alias
bridge against `alloca_coalescing.rs` and take the next bounded escape-analysis
gain around any remaining current-BIR escape sites that are still modeled only
as addressed-use bookkeeping instead of explicit escape classification,
especially if future pointer-preserving single-input forms should share the new
helper instead of silently depending on alias propagation.

## Watchouts

- the active C++ route now elides dedicated frame slots for dead local-slot
  objects and for prepared `copy_coalescing_candidate` lowering-scratch objects,
  while both addressed-local-slot roots and call-escaped local-slot roots now
  correctly stay live and address-exposed
- derived pointer aliases now keep local-slot roots live through the current
  BIR `CastInst` / `PhiInst` / `SelectInst` / pointer-shaped `BinaryInst`
  bridge, and `CastInst` now records an explicit escape instead of relying only
  on address-use bookkeeping plus alias propagation
- the C++ port still differs from `alloca_coalescing.rs` in that the Rust
  reference covers additional instruction families that the current BIR does
  not model, so keep future parity work tied to real active instruction shapes
  instead of speculative scaffolding
- `regalloc_helpers.cpp` still keeps most live local-slot objects conservative,
  so broader Rust-like alloca tiering is not active yet
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`
successfully; canonical proof log: `test_after.log`. No broader checkpoint was
run in this executor packet.
