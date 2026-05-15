# AArch64 CRC And Vector Intrinsic Carriers

Status: Open
Created: 2026-05-15

Parent Context: ideas/open/239_aarch64_intrinsic_machine_nodes.md

## Goal

Define the target-neutral semantic and prepared carrier authority required
before AArch64 CRC, vector memory, and vector operation intrinsics can be
selected into machine records.

## Why This Exists

The active AArch64 intrinsic machine-node route proved that scalar F32/F64
`fabs` has enough structured authority to select a machine record. CRC and
vector intrinsic selection blocked because the current intrinsic carrier model
only publishes `ScalarFpUnary` / `FAbs` facts. Ordinary call plans, intrinsic
names, archived scratch registers, and final assembly text are not valid
authority for CRC or vector selection.

This dependency must establish complete semantic and prepared facts before
the AArch64 selector can responsibly add CRC/vector records.

## In Scope

- Add intrinsic family and operation facts for accepted CRC, vector memory, and
  vector operation cases.
- Preserve feature requirements such as CRC availability and any vector
  feature gates required by accepted operations.
- Publish width, lane, element type, signedness where relevant, operand count,
  result identity, memory operand, immediate, and side-effect contracts.
- Carry register/allocation authority through prepared facts without relying on
  archived `w9`, `q0`, `q1`, or accumulator conventions.
- Add diagnostics and tests proving unsupported x86-only, missing-feature,
  missing-lane/type, missing-memory, and incomplete-carrier cases fail closed.
- Leave enough structured facts for `ideas/open/239_aarch64_intrinsic_machine_nodes.md`
  to later select and print AArch64 machine records without string matching.

## Out Of Scope

- Selecting or printing AArch64 machine records.
- Rewriting generic vector lowering, atomics, inline asm, binary128 helpers, or
  ordinary call lowering.
- Supporting all x86 SSE/AES/CLMUL intrinsics on AArch64.
- Emitting assembly text from intrinsic names or final call spelling.

## Acceptance Criteria

- BIR or equivalent semantic facts distinguish accepted CRC, vector memory, and
  vector operation intrinsic families from scalar unary and unsupported
  families.
- Prepared intrinsic carriers preserve all fields needed by AArch64 selection:
  feature requirements, lane/width/type facts, operand/result identities,
  memory/immediate contracts where applicable, side effects, and register
  authority.
- Prepared-printer or diagnostics expose those facts clearly enough for tests
  to prove completeness without implying selected machine support.
- Tests cover at least one complete representative route for each accepted
  carrier family and nearby incomplete or unsupported cases that must fail
  closed.
- The active AArch64 machine-node route can consume these carriers without
  falling back to intrinsic-name matching, ordinary call plans, or scratch
  register conventions.

## Reviewer Reject Signals

- A diff selects or prints CRC/vector AArch64 instructions before complete
  semantic and prepared intrinsic carrier facts exist.
- A diff matches named intrinsic strings, test fixture names, or final assembly
  spelling as the main source of CRC/vector authority.
- A diff treats ordinary `PreparedCallPlan` data as sufficient to select a
  CRC/vector intrinsic record.
- A diff downgrades unsupported-path expectations or weakens diagnostics
  without explicit user approval.
- A diff claims carrier progress through helper renames, expectation rewrites,
  or classification-only changes while lane/type/feature/operand/register
  authority remains missing.
- A diff broadens into unrelated generic vector, inline asm, atomic, binary128,
  or ordinary call rewrites instead of defining the missing intrinsic carriers.
- A diff preserves the old failure mode behind new type names, leaving
  CRC/vector selection dependent on spelling, scratch registers, or fabricated
  operands.
