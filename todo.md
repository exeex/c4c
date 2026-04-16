Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by adding an explicit conservative policy
for rooted-plus-unrooted pointer merges in the active C++ alloca/use bridge.
`alloca_coalescing.cpp` now treats `PhiInst`, `SelectInst`, and pointer-shaped
`BinaryInst` merges that combine a local-slot root with an unrooted pointer
value as immediate root escapes instead of relying on a later call/store use to
keep the slot live. `backend_prepare_stack_layout` now proves this with a
mixed-pointer select activation case alongside the earlier dead-slot,
copy-coalescing, addressed-local-slot, direct call-escaped local-slot, and
select-derived alias-escape coverage.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the remaining C++ pointer-alias
bridge against `alloca_coalescing.rs` and take the next bounded escape-analysis
gain around pointer-copy forms that still differ from the Rust reference,
especially whether `CastInst` or other pointer-preserving single-input routes
need a similarly deliberate conservative policy when provenance stops being
obvious.

## Watchouts

- the active C++ route now elides dedicated frame slots for dead local-slot
  objects and for prepared `copy_coalescing_candidate` lowering-scratch objects,
  while both addressed-local-slot roots and call-escaped local-slot roots now
  correctly stay live and address-exposed
- derived pointer aliases now keep local-slot roots live through the current
  BIR `CastInst` / `PhiInst` / `SelectInst` / pointer-shaped `BinaryInst`
  bridge, and mixed rooted-plus-unrooted pointer merges now conservatively keep
  the rooted slot live even when no later explicit escape site is reached
- the C++ port still differs from `alloca_coalescing.rs` in that the Rust
  reference treats more pointer-transform instructions as direct escape points,
  so the next slice should recheck single-input pointer transforms instead of
  assuming alias propagation alone is enough
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
