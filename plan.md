# RV64 Standard `.insn` Scalar Inline Asm Object Route

Status: Active
Source Idea: ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md
Activated from: ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md

## Purpose

Build the scalar `.insn` foundation required by the RV64 inline asm custom
vector encoding umbrella.

Goal: support a narrow, standard RV64 `.insn` inline-asm route for scalar GPR
operands and prove exact emitted bytes through c4c's object path.

## Core Rule

Implement a documented `.insn` lowering path, not a mnemonic-specific shortcut
or sample-string special case.

## Read First

- `ideas/open/346_rv64_standard_insn_scalar_inline_asm_object_route.md`
- `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`
- existing inline asm parser, HIR/LIR/BIR lowering, register substitution, and
  object emission tests before editing code

## Current Scope

- Standard RV64 scalar `.insn` forms, starting with `.insn r opcode, funct3,
  funct7, rd, rs1, rs2`.
- Scalar constraints: `r`, `=r`, `+r`, and supported matching/tied operands.
- GPR register substitution and field encoding.
- Volatile, memory, and clobber preservation along the supported route.
- Source diagnostics and object-byte tests.

## Non-Goals

- Do not implement `VR`, `VRM2`, `VRM4`, or vector group allocation here.
- Do not implement EV `.insn.d` here.
- Do not implement consteval/template-produced asm strings here.
- Do not add custom operation mnemonics or intrinsics.
- Do not broaden to full GNU assembler compatibility.
- Do not weaken existing inline asm or object expectations.

## Working Model

Treat `.insn` as a structured inline-asm template form. The compiler should
parse enough structure to bind allocated operands to fields, preserve inline
asm side-effect semantics, and emit the resulting instruction bytes through
the repo-native object route.

## Execution Rules

- Keep each step narrow enough for build plus focused tests.
- Prefer source-level diagnostics for unsupported or malformed `.insn` forms.
- Preserve existing behavior for non-`.insn` inline asm.
- Add negative tests beside positive capability tests.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.
- Escalate to broader backend/object validation before closure.

## Step 1: Map Existing Inline Asm And Object Surfaces

Goal: identify the exact parser, lowering, substitution, and object-emission
surfaces that Stage 1 must connect.

Primary targets:

- frontend inline asm AST/parser and semantic lowering
- HIR/LIR/BIR inline asm representation
- backend register substitution and prepared inline asm paths
- RV64 object emission and byte-inspection tests

Actions:

- Inspect current inline asm support and test coverage.
- Record where `.insn` templates currently pass through, fail, or lose
  structure.
- Identify the smallest object-route fixture that can prove emitted bytes.
- Update `todo.md` with the selected implementation route and proof command.

Completion check: `todo.md` records the selected route, target files, and
focused validation command for Step 2.

## Step 2: Parse And Represent Supported `.insn r`

Goal: carry a structured representation for the supported scalar `.insn r`
form without changing register allocation yet.

Actions:

- Parse the supported `.insn r opcode, funct3, funct7, rd, rs1, rs2` shape.
- Preserve ordinary inline asm behavior for unsupported templates.
- Diagnose malformed supported-shape attempts with source locations.
- Add focused positive and negative frontend/lowering tests.

Completion check: build passes and focused frontend/lowering tests prove that
supported `.insn r` templates are recognized while malformed forms diagnose.

## Step 3: Bind Scalar Constraints And Substitute GPR Operands

Goal: connect `r`, `=r`, `+r`, and supported tied operands to stable GPR field
substitution.

Actions:

- Ensure supported scalar constraints map to the existing GPR allocation path.
- Preserve tied and read-write operand semantics for the selected `.insn`
  route.
- Reject incompatible constraints with source-level diagnostics.
- Add tests for untied, tied, read-write, and incompatible scalar operands.

Completion check: focused inline asm/backend tests prove allocated scalar
operands substitute into the intended fields and invalid constraints reject.

## Step 4: Emit RV64 `.insn` Bytes Through Object Route

Goal: emit exact instruction bytes for the supported `.insn r` form without an
external assembler as the primary proof.

Actions:

- Encode opcode, funct3, funct7, rd, rs1, and rs2 into the RV64 instruction
  word.
- Route the encoded instruction through existing object emission.
- Add byte-level object tests for at least one scalar `.insn r` fixture.
- Keep disassembly-only checks secondary to byte proof.

Completion check: object-route tests prove exact emitted bytes for the chosen
fixture.

## Step 5: Preserve Side Effects And Close Diagnostics

Goal: make the supported route robust enough for closure review.

Actions:

- Verify `asm volatile`, memory, and clobber semantics are preserved.
- Add or tighten negative tests for unsupported `.insn` variants and malformed
  operands.
- Confirm existing inline asm, backend, and object tests were not weakened.
- Run a broader validation subset selected by the supervisor.

Completion check: focused tests plus the delegated broader subset pass, and
`todo.md` records any follow-up child ideas separately from this child.

## Step 6: Closure Review

Goal: decide whether this child source idea is complete.

Actions:

- Compare implementation and tests against the child acceptance criteria.
- Ensure remaining vector, EV `.insn.d`, and consteval/template work stays
  under separate child ideas.
- Run close-time regression guard if closure is requested.

Completion check: plan owner either closes this child after regression guard or
extends/replaces the runbook with a concrete remaining scalar `.insn` gap.
