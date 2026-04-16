Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by teaching the active C++ alloca/use bridge
to treat `MemoryAddress::PointerValue` bases as rooted local-slot pointer uses.
`alloca_coalescing.cpp` now records pointer-based load/store addresses through
the same alias-aware root lookup used for other pointer values, and
`backend_prepare_stack_layout` proves that a cast-derived pointer base keeps
the root local slot address-exposed and home-slotted when the address flows
through a BIR memory access instead of a direct local-slot base.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the remaining C++ pointer-alias
bridge against `alloca_coalescing.rs` and take the next bounded parity gain in
how pointer-preserving control-flow/value forms are classified, especially if a
current-BIR escape site still depends on ad hoc addressed-use bookkeeping
instead of sharing one explicit alias-to-root escape/use path.

## Watchouts

- the active C++ route now treats both direct `LocalSlot` addresses and
  pointer-based `MemoryAddress::PointerValue` bases as real local-slot uses, so
  cast-derived pointer bases keep rooted local slots live even when the memory
  op no longer names the slot directly
- derived pointer aliases still keep local-slot roots live through the current
  BIR `CastInst` / `PhiInst` / `SelectInst` / pointer-shaped `BinaryInst`
  bridge, but the remaining Rust-vs-C++ comparison should stay tied to real
  current-BIR instruction/address forms instead of speculative scaffolding
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
