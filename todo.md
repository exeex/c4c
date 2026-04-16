Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by teaching the active C++ alloca/use bridge
to treat indirect-call `callee_value` pointers as rooted local-slot escapes.
`alloca_coalescing.cpp` now routes indirect callees through the same alias-aware
pointer-use path as call arguments and pointer-based memory addresses, and
`backend_prepare_stack_layout` proves that a cast-derived callee alias keeps
the root local slot address-exposed and home-slotted when the pointer escapes
through an indirect BIR call target.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the remaining C++ pointer-alias
bridge against `alloca_coalescing.rs` and take the next bounded parity gain in
how pointer-preserving control-flow/value forms are classified, especially if a
current-BIR root escape or use path is still open-coded instead of sharing one
explicit alias-to-root helper across call, memory-address, terminator, and
pointer-transform sites.

## Watchouts

- the active C++ route now treats direct `LocalSlot` addresses, indirect-call
  `callee_value` pointers, and pointer-based `MemoryAddress::PointerValue`
  bases as real local-slot uses, so cast-derived pointer aliases keep rooted
  local slots live even when the use site no longer names the slot directly
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
