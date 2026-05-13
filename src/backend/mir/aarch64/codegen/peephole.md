# AArch64 Peephole Legacy Surface

This artifact preserves the useful production shape from the removed
`peephole.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/peephole.rs`.

## Role

The surface implemented an AArch64 assembly-text peephole optimizer.

It operated after stack-based code generation had emitted textual assembly.
The optimizer split the assembly into lines, classified each line into a compact
`LineKind`, ran a bounded set of local and global text rewrites, then rendered
all non-`Nop` lines back to assembly text.

The design was intentionally text-first. It did not own MIR lowering, register
allocation, frame layout, relocation handling, or instruction encoding. Its job
was cleanup of redundant stack traffic, simple branch patterns, and short
register-copy chains produced by earlier AArch64 codegen.

## Entry Point

- `peephole_optimize(asm: String) -> String`: whole-pipeline entry. It
  classified all lines once, ran iterative local passes, ran global copy and
  dead-store passes, performed a final local cleanup loop, and returned the
  surviving assembly lines.

The pipeline used tombstone metadata instead of physically deleting lines while
passes were running. `LineKind::Nop` was the deletion signal, and final rendering
filtered those tombstones.

## Line Model

`LineKind` encoded just enough instruction shape for the peephole passes:

- blank/deleted lines: `Nop`
- stack traffic: `StoreSp`, `LoadSp`, `LoadswSp`, `StorePairSp`, `LoadPairSp`
- general byte/halfword or non-stack memory traffic: `LoadsbReg`, `MemOther`
- moves: `Move`, `MoveImm`, `MoveWide`
- sign extension: `Sxtw`
- control flow: `Branch`, `CondBranch`, `CmpBranch`, `Label`, `Ret`, `Call`
- other categories: `Compare`, `Alu`, `Directive`, `Other`

Register parsing mapped `x0..x30` and `w0..w30` to the same numeric ids,
reserved `31` for `sp`, `32` for `xzr`/`wzr`, and `255` as `REG_NONE`.

The classifier was deliberately shallow string parsing. It recognized common
AArch64 mnemonics and stack-address shapes such as `[sp]` and `[sp, #N]`, while
falling back to conservative categories for pre-indexing, indirect memory, pair
loads/stores, calls, indirect branches, and unknown instructions.

## Pass Schedule

The old pipeline had three stages:

1. Local iterative cleanup, up to eight rounds:
   - adjacent store/load elimination
   - redundant branch elimination
   - 64-bit self-move elimination
   - move chain optimization
   - branch-over-branch fusion
2. Global passes:
   - register-copy propagation
   - dead stack-store elimination
3. Final local cleanup, up to four rounds:
   - adjacent store/load elimination
   - redundant branch elimination
   - 64-bit self-move elimination
   - move chain optimization

The old comments still described global store forwarding, but the actual
pipeline had that pass disabled because of a known correctness bug in complex
float-array code, specifically noted around testcase `0036_0041`.

## Local Rewrite Contracts

### Adjacent Store/Load Elimination

This pass looked past tombstones to a neighboring stack reload after a store:

- `str xN, [sp, #off]` followed by `ldr xN, [sp, #off]` deleted the load.
- `str xN, [sp, #off]` followed by `ldr xM, [sp, #off]` rewrote the load to
  `mov xM, xN`.
- `str wN, [sp, #off]` followed by `ldrsw xM, [sp, #off]` rewrote the load to
  `sxtw xM, wN`.

The rewrite was width-sensitive. Word and doubleword stack traffic were not
treated as interchangeable.

### Redundant Branch Elimination

An unconditional `b .label` was removed when the next non-`Nop` line was that
exact label. This was fallthrough cleanup only; it did not reason about larger
control-flow structure.

### Self-Move Elimination

Only `mov xN, xN` was removed. `mov wN, wN` was preserved because AArch64
32-bit register writes zero the upper half of the corresponding `x` register.

### Move Chain Optimization

The pass rewrote immediate and register copy chains over the next active line:

- `mov A, B; mov C, A` became `mov C, B` when both moves had the same width and
  the rewrite did not create a direct self-copy.
- `mov xN, #imm; mov xM, xN` could retarget the immediate into `xM` when `xN`
  was one of the scratch registers `x0..x15`.
- `movz` and `movn` were treated as fresh immediate definitions; `movk` was not,
  because it updates part of an existing register value.

### Branch-Over-Branch Fusion

The pass recognized this shape:

```asm
b.cc .Lskip
b .target
.Lskip:
```

When the inverse condition was known and the estimated conditional-branch
distance was below a conservative threshold, it rewrote the first branch to
`b.!cc .target` and tombstoned the unconditional branch.

Supported condition inversions included equality, signed comparisons, unsigned
comparisons, sign, overflow, and carry aliases such as `hs`/`cs` and `lo`/`cc`.

## Global Pass Contracts

### Register-Copy Propagation

The global copy pass considered only 64-bit `mov` aliases where both registers
were below `x29`. It looked at the next active instruction, skipped labels,
branches, returns, directives, calls, pair loads, exclusive loads/stores, and
CAS-like operations, then attempted to replace source-register references.

The pass delegated whole-word textual replacement to shared peephole helpers in
`backend::peephole_common`, including:

- `replace_source_reg_in_instruction`
- `replace_whole_word` for tests

The dependency on whole-word replacement was important. The old tests guarded
against replacing `x1` inside `x10` or `x11`.

### Dead Stack-Store Elimination

The dead-store pass first bailed out if any active instruction appeared to take
the address of `sp`, including:

- `add xN, sp, #off`
- `sub xN, sp, #off`
- `mov xN, sp`

If the frame address was not exposed, it collected byte ranges loaded from stack
slots and removed stack stores whose byte ranges did not overlap any load. The
byte-range model mattered because a wide store can be read by a narrower load
at a partially overlapping offset.

The pass recognized load sizes for common `ldr`, `ldur`, sign-extending,
byte/halfword, and pair-load forms. Unknown or pointer-exposed cases stayed
conservative.

## Hidden Assumptions

- Input text is AArch64 assembly in the backend's own formatting style.
- Simple stack-slot traffic uses `[sp]` or `[sp, #N]`; pre-indexed or complex
  addressing is not treated as a normal stack slot.
- Tombstone metadata, not source text mutation, is authoritative while passes
  run.
- Labels may still be externally targeted, so branch-over-branch fusion keeps
  the skip label even after removing the intervening branch.
- `mov wN, wN` has semantic value because it zero-extends into `xN`.
- Calls, returns, indirect branches, directives, pair operations, exclusive
  memory instructions, and CAS operations are barriers for the simple copy
  propagation model.
- Dead-store elimination is only sound when the frame address is not exposed.

## Tests Preserved In The Old Surface

The removed comments carried unit tests for:

- classifying stores, loads, `ldrsw`, moves, immediates, branches, labels,
  returns, and `sxtw`
- adjacent store/load deletion and forwarding
- redundant branch deletion
- 64-bit self-move deletion
- branch-over-branch fusion
- register and immediate move chains
- word-store to `ldrsw` fusion
- disabled global-store-forwarding cases
- copy-propagation whole-word matching
- dead-store preservation when a stack address is taken

The disabled GSF tests are evidence of desired behavior, not proof that the old
implementation was safe enough to enable that pass.

## Dependencies

The old surface depended on:

- AArch64 textual assembly emitted by the codegen layer.
- Stable backend formatting for registers, stack addresses, labels, and branch
  mnemonics.
- Shared peephole string helpers for source-register replacement.
- A final renderer that filters `LineKind::Nop` lines.

No live CMake metadata referenced this legacy AArch64 `.cpp`; the corresponding
module file only retained a commented Rust-style `peephole` module line.

## Rebuild Risks

- Re-enabling global store forwarding without a stronger alias/control-flow
  model can revive the documented complex float-array miscompile.
- Treating `w`-register self-moves as no-ops would lose required zero-extension.
- Naive textual replacement can corrupt register names such as replacing `x1`
  inside `x10` or `x11`.
- Dead-store elimination must stay conservative when stack addresses escape or
  when loads overlap stores at different byte widths.
- Branch fusion must respect conditional-branch range and must not delete labels
  that may have other incoming branches.
- Copy propagation must treat calls, labels, indirect control flow, exclusive
  memory instructions, pair loads, and multi-destination forms as barriers.

## Future Ownership Boundary

A rebuilt AArch64 peephole route should own only post-codegen assembly cleanup.
It should not become a place for semantic lowering, frame-layout repair,
register-allocation policy, ABI decisions, encoder work, or testcase-shaped
recovery. If stronger optimization is needed, prefer a structured assembly or
CFG-aware representation over expanding raw string heuristics.
