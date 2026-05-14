# AArch64 Inline Assembly Machine Nodes

Status: Open
Created: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

The archived `inline_asm.md` describes GCC-style AArch64 inline assembly
operand substitution, modifiers, raw immediates, named operands, positional
operand numbering, and atomic helper routines. Current lowering preserves
inline-asm payload text as BIR call metadata and prepared stack layout observes
side effects, but compiled AArch64 codegen does not consume constraints,
operands, clobbers, output homes, or template substitution into selected
machine nodes or final assembler text.

Inline assembly is a real external/textual boundary, but its backend contract
must be structured before formatting. It should not recover semantics from
already-rendered assembly text.

## Scope

- Define structured inline-asm operand, constraint, clobber, side-effect,
  output, tied-operand, immediate, and named-operand carriers.
- Lower AArch64 inline-asm machine records from those carriers while consuming
  prepared register and stack-home authority.
- Implement AArch64 template substitution for supported modifiers only after
  operands have assigned structured homes.
- Preserve diagnostics for unknown operands and unsupported constraints.

## Non-Goals

- Do not rebuild an independent inline-asm register allocator or spill planner.
- Do not use the AArch64 assembler parser to recover codegen semantics from
  printed text.
- Do not fold atomic exclusive helper selection into inline-asm template
  substitution.
- Do not claim support through a single hard-coded inline-asm fixture.

## Proof Direction

- Positional, named, tied, and immediate operands substitute from structured
  records with correct AArch64 modifier spelling.
- Side-effect and clobber facts influence prepared homes and selected machine
  records without local scratch-register shortcuts.
- Unknown operands or unsupported constraints produce explicit diagnostics
  instead of fabricated registers.
