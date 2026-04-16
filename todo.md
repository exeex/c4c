Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by adding focused
`backend_prepare_stack_layout` proof for the remaining active current-BIR
pointer-merge and pointer-transform shapes: a `PhiInst` alias path and a
pointer-valued `BinaryInst` alias path. The active C++
`alloca_coalescing.cpp` helper already kept those rooted local-slot aliases
address-exposed and home-slotted when they later escaped, so the packet landed
as runtime-proof coverage rather than a semantic code fix.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the active C++
`alloca_coalescing.cpp` helper responsibilities against
`stack_layout/alloca_coalescing.rs` now that the current-BIR pointer-address,
pointer-merge, and pointer-transform shapes have focused runtime proof, and
take the next bounded packet only where that comparison exposes a real
semantic gap or a missing active-shape proof.

## Watchouts

- the active C++ route now has focused proof for return, pointer-typed
  cond-branch, call-operand, indirect-callee, pointer-address, select, cast,
  store-of-pointer, `PhiInst`, and pointer-valued `BinaryInst` escape sites,
  but Step 2 acceptance still needs the broader `.cpp` vs `.rs` comparison
  rather than treating testcase coverage as convergence by itself
- derived pointer aliases still keep local-slot roots live through the current
  BIR `CastInst` / `PhiInst` / `SelectInst` / pointer-shaped `BinaryInst`
  bridge, but the remaining Rust-vs-C++ comparison should stay tied to real
  active instruction/address forms instead of speculative scaffolding
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
