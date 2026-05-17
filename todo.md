Status: Active
Source Idea Path: ideas/open/265_aarch64_asm_emitter_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Shard Against The Live Owner

# Current Packet

## Just Finished

Step 1 - Audit The Shard Against The Live Owner completed.

`src/backend/mir/aarch64/codegen/asm_emitter.md` is a legacy inline-assembly
surface, not the current compiled-module-to-`.s` owner. Current facts worth
preserving are limited to route guidance: final assembly text should be a
printer consumer of structured target data, inline-assembly behavior should be
rebuilt through semantic MIR/constraint ownership before printing, and legacy
assembly-text/parser recovery should not become a backend bridge.

Obsolete material to reject from the shard: direct `InlineAsmEmitter` behavior,
GCC constraint classification, scratch register allocation, operand preload and
output store machinery, `%l[...]` template substitution, fixed `x9` bridge
paths, condition-code output scaffolding, AArch64 logical-immediate validators,
and all old Rust-reference dependency names. Those belong to future structured
inline-asm work, not this asm emitter redistribution.

Facts already represented by the live owner: `asm_emitter.hpp` exposes the thin
`print_prepared_machine_nodes(const PreparedBirModule&)` public surface;
`asm_emitter.cpp` compiles prepared BIR into an AArch64 machine module, rejects
failed or empty selected-node routes, emits `.text`, `.globl`, `.type`, labels,
`.size`, and `.note.GNU-stack`, and delegates instruction payload spelling to
`MachineInstructionPrinter` through the shared MIR printer. `backend.cpp` only
holds the public AArch64 BIR/LIR handoff calls into that owner.

Concrete Step 2 ownership issue: no stranded current-route assembly rendering
helper was found in broader owners. `MachineInstructionPrinter` remains the
correct sibling owner for instruction spelling, while `asm_emitter.cpp` owns
translation-unit assembly wrapping and prepared-module printing.

## Suggested Next

Start Step 2 by confirming the no-move result in code review terms: keep
`asm_emitter.cpp`/`.hpp` as the module `.s` owner, leave
`machine_printer.*` as instruction spelling owner, and make no behavior changes
unless a supervisor-selected proof reveals a missed current-route helper.

## Watchouts

- Preserve emitted `.s` behavior.
- Do not port legacy inline-assembly reference machinery.
- Do not redistribute neighboring AArch64 markdown shards.
- Do not weaken tests or smoke coverage.
- Do not move `machine_printer.*` instruction-spelling logic into
  `asm_emitter.cpp`; that would blur the current owner boundary.

## Proof

Not run; delegated proof explicitly said no build required for this audit-only
packet. No `test_after.log` was produced because there was no proof command to
capture.
