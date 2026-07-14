# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.21
Current Step Title: Publish scalar inline-asm output binding authority

## Just Finished

- Stopped Plan Step 7.20 without code changes and classified builtin parity as
  an exact separately owned blocker.
- Existing native Ctpop, generic scalar integer BinOp, exact Trunc, and generic
  ownership seams can publish the call, And-with-one, optional narrowing, and
  final-use IDs. They cannot make reachable verification reject parity's And
  being mutated to another otherwise-valid integer opcode.
- Closing that semantic gap requires a new native parity-purpose carrier owned
  by a separate future initiative. Producer/name/text inference is forbidden,
  and idea 744 must not silently absorb that semantic-carrier design.
- Step 7 is still active: the checked matrix explicitly marks semantic
  inline-asm outputs as able to use generic result allocation and ownership,
  while keeping opaque assembly payload separate.

## Suggested Next

- Executor: complete Plan Step 7.21 for only PS's scalar integer output-only
  `LirInlineAsmOp` route. Allocate the output through `fresh_value`, store that
  exact operand in the semantic output/result binding, and preserve the exact
  output ID into one later ordinary scalar use.

## Watchouts

- Own only one output-only scalar integer binding whose existing native
  type/role/output-index facts are complete. Preserve their exact agreement
  with the semantic results collection and the later use.
- The compatibility `result`, rendered operands, original assembly and
  constraint text, clobbers, and all other rendered mirrors remain
  presentation/opaque. They may observe native authority but never create,
  repair, select, or validate it.
- Reachable verification must reject invalid or duplicate output definitions;
  missing, wrong-alternative, unknown, or cross-function result/use IDs; and
  conflicting binding type, role, output index, collection position, or later
  scalar-use type. Misleading displays with unchanged native facts must pass.
- Exclude input, tied/read-write, memory, address, immediate, clobber, explicit
  register, vector, and multi-output bindings; `insn_r` semantics; opaque-text
  interpretation; compatibility-result authority; stack/local/object and body
  parameter publication; CFG; BIR receipt; and all idea-741 contracts.
- This packet is carrier-ready only on the existing semantic output binding,
  scalar type/role/index facts, generic `fresh_value`, and ownership verifier.
  If the selected output-only route needs a new binding-semantic carrier or
  text-derived bridge, stop and return that exact blocker rather than widening
  Step 7.21.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`
  with an output-only scalar identity chain, misleading-display positives, and
  malformed native result, binding-fact, and final-use cases.
- Preserve neighboring inline-asm metadata/diagnostic behavior and accepted
  generic scalar identity coverage unchanged; the supervisor owns matched
  regression logs and any broader/full proof.
- Run `git diff --check` before handoff.
