Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Continued Step 2 in `stack_layout` by making the active C++
`alloca_coalescing` terminator path classify rooted pointer operands through an
explicit escape helper instead of the generic ordinary-use helper. The focused
`backend_prepare_stack_layout` coverage now includes a real current-BIR return
terminator shape where a pointer-typed local slot escapes only by being
returned, and the active C++ path still leaves that root slot address-exposed
and home-slotted.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the remaining C++ explicit-escape
sites against `alloca_coalescing.rs` and take the next bounded parity gain in
how current-BIR call/store-like pointer operands are classified, especially
where the active C++ path still records rooted pointer aliases as ordinary uses
instead of routing them through the same escape-focused helper shape now used
for terminators.

## Watchouts

- the active C++ route now treats return and conditional-branch terminator
  operands through an explicit pointer-escape helper, but current-BIR coverage
  only proves the return shape; pointer-typed branch conditions remain unusual
  and still unproven by focused tests
- derived pointer aliases still keep local-slot roots live through the current
  BIR `CastInst` / `PhiInst` / `SelectInst` / pointer-shaped `BinaryInst`
  bridge, but the remaining Rust-vs-C++ comparison should stay tied to real
  active instruction/address forms instead of speculative scaffolding
- the active C++ path still has several call/store-like sites that classify
  rooted pointer aliases through the generic ordinary-use helper rather than
  the newer explicit-escape helper shape, so future parity work should keep the
  distinction bounded and observable
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
