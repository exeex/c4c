# RV64 Standard `.insn` Inline Asm Stage 1

## Goal

Implement the first child of
`ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`: standard
RV64 `.insn` inline asm with scalar constraints and object-route proof.

This child establishes the target-owned inline-asm path that later vector
constraint, EV `.insn.d`, and consteval-template string work must build on.

## Why This Exists

The umbrella should not start with EV vector encodings. c4c first needs a
standard RISC-V `.insn` route that can parse, carry, substitute, allocate, and
emit ordinary scalar inline asm without delegating the proof to an external
assembler.

Stage 1 creates that baseline for current RV64 instruction lengths and scalar
operand constraints.

## In Scope

- Parse and preserve inline asm templates containing standard RISC-V `.insn`
  forms.
- Support the scalar constraints needed for the first RV64 proof:
  - `r`
  - `=r`
  - `+r`
  - matching or tied operands where the current inline-asm path already
    supports them or the selected `.insn` proof requires them.
- Substitute allocated scalar registers into positional placeholders such as
  `%0`, `%1`, and `%2`.
- Preserve `asm volatile`, clobber, and memory-side-effect metadata.
- Emit or carry resulting instruction bytes through c4c's object route.
- Add focused tests for successful `.insn`, malformed `.insn`, unsupported
  constraints, substitution, and emitted bytes.

## Out Of Scope

- `VR`, `VRM2`, `VRM4`, or any other vector register constraint.
- EV-specific `.insn.d` syntax or 64-bit custom instruction encoding.
- GNU named operands such as `[name] "r"(x)` and `%c[name]` modifiers.
- Runtime-generated asm template strings.
- Teaching c4c semantic meanings for EV custom operations.
- Broad RVV lowering or vector value semantics.
- Proving success by routing the primary object output through an external
  assembler.

## Dependency Notes

- Parent umbrella:
  `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`.
- Later children should not start until this child defines the RV64 inline-asm
  carrier, substitution, and object proof contract clearly enough to extend.
- If implementation discovers that generic inline-asm carrier work is missing
  before RV64 `.insn` can be attempted, keep that work inside this child only
  when it is directly required for the Stage 1 `.insn` path. Otherwise create a
  separate child idea.

## Acceptance Criteria

- A focused RV64 source case using:

  ```cpp
  asm volatile(".insn r 0x0b, 0, 0, %0, %1, %2"
               : "=r"(out)
               : "r"(a), "r"(b));
  ```

  compiles through the intended inline-asm lowering path.
- The compiler preserves volatile, constraint, operand ordering, and clobber or
  memory metadata relevant to the chosen proof.
- Scalar GPR constraints allocate legal RV64 registers and substitute stable
  register spellings into `.insn` placeholders.
- Object-route proof checks emitted bytes against a known standard RISC-V
  encoding without depending on an external assembler as the primary proof.
- Negative tests reject malformed `.insn` fields and unsupported constraints
  clearly.
- Existing backend tests are not weakened to make the route appear supported.

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

- The route only passes by marking `.insn` tests unsupported or weakening
  existing backend expectations.
- The implementation recognizes only the exact sample string instead of a
  documented standard `.insn` operand shape.
- Object proof depends primarily on an external assembler rather than c4c's
  target-owned object route.
- Scalar constraints are accepted as raw strings but never participate in real
  allocation or substitution.
- Volatile, clobber, or memory-side-effect metadata is dropped while claiming
  inline-asm support.
- The change adds vector, EV `.insn.d`, named-operand, or consteval-template
  string behavior before the Stage 1 scalar `.insn` path is proven.
- Existing special cases are renamed or rearranged while the same `.insn`
  failure mode remains.

## Lifecycle Note

2026-06-24: Stage 1 implementation and runbook work are complete against the
acceptance criteria. Closure was not accepted only because the strict
before/after regression close gate rejected the unchanged backend baseline:
both `test_before.log` and `test_after.log` reported 315/316 passing with the
known unrelated
`backend_codegen_route_riscv64_pointer_typed_select_publication` failure. The
active runbook is parked rather than expanded into that unrelated baseline
issue.
