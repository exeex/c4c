# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.22
Current Step Title: Publish scalar multi-output inline-asm binding authority

## Just Finished

- Accepted Plan Step 7.21 for only PS's scalar integer output-only
  `LirInlineAsmOp` semantic binding and later Store use.
- The output is allocated through `fresh_value`, retained as one exact native
  type/Output-role/index-zero binding definition, and preserved into its
  type-matched Store while compatibility and opaque text stay authority-free.
- Fresh focused and matched full proof passed, and commit `91bfc1715` records
  the coherent Step-7.21 slice.
- Step 7 is not complete: the checked matrix still lists scalar multi-output
  bindings as unclaimed, and that neighboring row is carrier-ready on the same
  native binding and generic ownership seams.
- Builtin parity remains an exact separately owned blocker requiring a future
  native parity-purpose semantic carrier; it is not reopened by this packet.

## Suggested Next

- Executor: complete Plan Step 7.22 for only PS's output-only scalar integer
  multi-output `LirInlineAsmOp` route. Allocate each semantic output through
  `fresh_value`, preserve distinct exact IDs in the corresponding ordered
  result bindings, and carry each output into a later type-matched ordinary
  Store use.

## Watchouts

- Own one output-only statement with two scalar integer outputs. Require one
  distinct function-owned definition per output and exact agreement among
  native binding type, Output role, output index, results collection position,
  and the corresponding later Store use.
- Preserve source order and require exact output indices zero and one; never
  merge, swap, duplicate, or recover output identity from rendered constraint,
  operand, result, or temporary spelling.
- Compatibility `result`, rendered operands, original assembly/constraint
  text, clobbers, and all other rendered mirrors remain presentation/opaque.
  They may observe native authority but may not create, repair, select, or
  validate it.
- Reachable verification must reject invalid, duplicate, missing, aliased,
  swapped, wrong-alternative, unknown, or cross-function output/use IDs and
  conflicting binding type, role, index, position, count, or Store-use type.
  Misleading displays with unchanged native facts must pass.
- Exclude single-output Step 7.21, inputs, tied/read-write, memory, address,
  immediate, clobber, explicit-register, vector, and mixed-type bindings;
  `insn_r` semantics; opaque-text interpretation; compatibility-result
  authority; stack/local/object and body parameters; CFG; BIR receipt; parity;
  and all idea-741 contracts.
- This packet is carrier-ready only on repeated use of the accepted semantic
  output binding, scalar type/role/index facts, `fresh_value`, and ownership
  verifier. If multi-output order/count needs a new semantic carrier or any
  text-derived bridge, stop and return that exact blocker rather than widening
  Step 7.22.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`
  with a two-output scalar identity chain, misleading-display positives, and
  malformed distinctness, order/index/count, binding-fact, and final-use cases.
- Preserve Step-7.21 single-output behavior, neighboring inline-asm metadata/
  diagnostics, and accepted generic scalar identity coverage unchanged; the
  supervisor owns matched regression logs and any broader/full proof.
- Run `git diff --check` before handoff.
