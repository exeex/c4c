Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by routing the active C++
`alloca_coalescing` call operands and stored pointer values through the same
explicit escape helper shape already used for terminators. The focused
`backend_prepare_stack_layout` coverage now also includes a real current-BIR
store-of-pointer-value shape where a root local slot escapes by being stored,
and the active C++ path still leaves that root slot address-exposed and
home-slotted.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the remaining current-BIR
pointer-address sites against `alloca_coalescing.rs` and take the next bounded
parity gain where rooted pointer operands are still treated as generic uses,
especially `MemoryAddress::PointerValue` forms and any other active address
base shapes that should share the explicit escape/helper split without
speculative scaffolding.

## Watchouts

- the active C++ route now treats return/conditional-branch terminators, call
  operands, and stored pointer values through an explicit pointer-escape
  helper, but current-BIR coverage still does not prove the unusual
  pointer-typed branch-condition shape
- derived pointer aliases still keep local-slot roots live through the current
  BIR `CastInst` / `PhiInst` / `SelectInst` / pointer-shaped `BinaryInst`
  bridge, but the remaining Rust-vs-C++ comparison should stay tied to real
  active instruction/address forms instead of speculative scaffolding
- `MemoryAddress::PointerValue` still routes through the generic use helper, so
  the next parity pass should decide explicitly whether active address-base
  shapes should remain ordinary uses or join the escape helper path
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
