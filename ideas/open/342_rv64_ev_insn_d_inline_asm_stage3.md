# RV64 EV `.insn.d` Inline Asm Stage 3

## Goal

Implement the third child of
`ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`: a
c4c/EV-specific 64-bit `.insn.d` inline asm template route for custom vector
instructions.

This child builds on the completed Stage 1 scalar `.insn` route and completed
Stage 2 vector register constraints by adding fixed 64-bit instruction
encoding for EV custom operations.

## Why This Exists

The umbrella goal is to let user/library code define custom vector instruction
families without c4c growing built-in mnemonics for every operation. Stage 1
proved the standard RV64 `.insn` carrier and object route. Stage 2 proved
compiler-owned vector register classes, grouped allocation, overlap rules, and
base-register substitution.

Stage 3 should use those foundations to encode an EV 64-bit instruction form
from inline asm operands and compile-time immediate fields.

## In Scope

- Parse and lower positional inline asm templates using `.insn.d`.
- Support the first EV 64-bit template shape with register operands and fixed
  compile-time immediate fields.
- Accept scalar and vector operands according to the already-supported RV64
  inline asm constraints, especially `VRM2` where useful for the first proof.
- Substitute allocated registers into `.insn.d` positional placeholders.
- Encode the resulting 64-bit instruction bytes through c4c's target-owned
  object route.
- Reject malformed `.insn.d` fields, non-constant immediate fields, unsupported
  operand classes, and values outside the selected bit-field widths.
- Add focused tests for successful encoding, emitted bytes, malformed syntax,
  immediate range failures, unsupported constraints, and register
  substitution.

The preferred first user-facing shape is positional:

```cpp
#define EV64  0x0a
#define EVADD 0x0b

asm volatile(".insn.d %4, %5, %0, %1, %2, %3, %6"
             : "=VRM2"(vd)
             : "VRM2"(a), "VRM2"(b), "VRM2"(c),
               "i"(EV64), "i"(EVADD), "i"(EV_DTYPE_I32));
```

## Out Of Scope

- GNU named operands such as `[major] "i"(EV64)` and `%c[major]` modifiers.
- Runtime-generated asm template strings.
- Consteval or template-built asm strings.
- Vector mask-specific constraints or policy semantics beyond fixed immediate
  fields.
- Broad RVV lowering or vector language semantics.
- Teaching c4c semantic meanings for EV operations such as `EVADD`.
- Linker-visible relocations inside the 64-bit instruction.
- Reworking the completed Stage 1 or Stage 2 routes except where `.insn.d`
  directly reuses their carrier, substitution, allocation, or object plumbing.

## Dependency Notes

- Parent umbrella:
  `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`.
- Completed scalar foundation:
  `ideas/open/340_rv64_standard_insn_inline_asm_stage1.md`.
- Completed vector foundation:
  `ideas/closed/341_rv64_vector_register_inline_asm_constraints_stage2.md`.
- Stage 1 is parked as implementation-complete but still open because its
  strict close gate was blocked by an unchanged unrelated backend baseline
  failure. Treat its scalar inline asm route as available foundation, not as
  active work to reopen.
- Stage 2 is archived and should be treated as the completed vector register
  constraint foundation for this child.

## Acceptance Criteria

- A focused RV64 source case using positional `.insn.d` compiles through the
  intended inline asm lowering path.
- Scalar and vector register operands use the existing allocation and
  substitution contracts; grouped vector operands encode by base register.
- Compile-time immediate operands populate the selected EV 64-bit fields and
  reject non-constant or out-of-range values clearly.
- Object-route proof checks the emitted 64-bit instruction bytes against the
  documented EV template layout without using an external assembler as the
  primary proof.
- Negative tests reject malformed `.insn.d` templates, unsupported operand
  classes, invalid immediate widths, and missing compile-time immediates.
- Existing Stage 1 `.insn` and Stage 2 vector constraint behavior is not
  weakened to make Stage 3 appear supported.

## Suggested Proof Ladder

- Build:

  ```bash
  cmake --build --preset default
  ```

- Narrow backend proof:

  ```bash
  ctest --test-dir build -j --output-on-failure -R '^backend_'
  ```

- Add frontend/HIR proof if the packet changes parser, HIR, or LIR inline-asm
  carrier behavior:

  ```bash
  ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_cxx_'
  ```

## Reviewer Reject Signals

- The route only passes by marking `.insn.d` tests unsupported or weakening
  existing `.insn` or vector constraint expectations.
- The implementation recognizes only the exact sample string instead of a
  documented positional `.insn.d` operand shape.
- Object proof depends primarily on an external assembler rather than c4c's
  target-owned object route.
- Register operands are accepted as raw strings but bypass real allocation,
  grouped-vector base substitution, or overlap rules.
- Immediate operands are accepted without compile-time validation, width/range
  diagnostics, or deterministic bit placement.
- The change adds named operands, `%c[...]` modifiers, mask policy semantics,
  consteval strings, or broad RVV lowering before the positional `.insn.d`
  route is proven.
- Existing special cases are renamed or rearranged while the same `.insn.d`
  encoding failure mode remains.
