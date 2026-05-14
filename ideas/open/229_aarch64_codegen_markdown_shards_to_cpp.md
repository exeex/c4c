# AArch64 Codegen Markdown Shards To Cpp

Status: Open
Created: 2026-05-14
Parent Context: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md

## Intent

Convert each legacy markdown shard under
`src/backend/mir/aarch64/codegen/*.md` into real compiled C++ implementation,
one shard at a time, with a matching backend C fixture proof for every
converted shard.

The markdown files are not completion artifacts. They are archived production
surface notes that must be turned into active `.cpp` / `.hpp` code only when
the corresponding behavior is wired into the current AArch64 MIR/codegen route
and proved by a focused `tests/backend/case/xxxx.c` case.

## Source Markdown Set

This idea owns the conversion of these source artifacts:

- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/asm_emitter.md`
- `src/backend/mir/aarch64/codegen/atomics.md`
- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/emit.md`
- `src/backend/mir/aarch64/codegen/f128.md`
- `src/backend/mir/aarch64/codegen/float_ops.md`
- `src/backend/mir/aarch64/codegen/globals.md`
- `src/backend/mir/aarch64/codegen/i128_ops.md`
- `src/backend/mir/aarch64/codegen/inline_asm.md`
- `src/backend/mir/aarch64/codegen/intrinsics.md`
- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/mod.md`
- `src/backend/mir/aarch64/codegen/peephole.md`
- `src/backend/mir/aarch64/codegen/prologue.md`
- `src/backend/mir/aarch64/codegen/records.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/variadic.md`

## Conversion Rule

For each markdown shard:

1. Create or update the corresponding compiled C++ surface under
   `src/backend/mir/aarch64/codegen/`.
2. Wire it through build files and the current AArch64 MIR/codegen ownership
   route.
3. Add or reuse one explicit fixture under `tests/backend/case/xxxx.c` that
   exercises the converted capability.
4. Register a CTest proof for that fixture through the backend/MIR test owner
   that naturally owns the route.
5. Record the proof command and result in `todo.md` before claiming that shard
   converted.

The converted `.cpp` must own real semantic behavior or a real structural
boundary. Merely preserving the markdown text as comments, adding a stub file,
or changing expectations without capability progress does not count.

## Proof Ledger

Every completed shard needs a ledger entry in the active runbook or `todo.md`
with this shape:

```text
Shard: src/backend/mir/aarch64/codegen/<name>.md
Converted Code: src/backend/mir/aarch64/codegen/<name>.cpp[, <name>.hpp]
Proof Fixture: tests/backend/case/<case>.c
Proof Test: <ctest name or exact ctest -R selector>
Result: <fresh passing result>
```

Suggested fixture naming is `aarch64_codegen_<shard>*.c`, but the proof may
use a clearer feature name when the C behavior is more specific than the shard
name. Each converted shard must have its own explicit ledger mapping to a
`tests/backend/case/xxxx.c` fixture before the shard is marked complete.

## Expected Slice Order

Prefer converting lower-risk structural or already-active surfaces first, then
move into feature families:

- active route and record boundaries: `emit`, `records`, `mod`
- existing selected-machine-node behavior: `alu`, `comparison`, `returns`,
  `memory`
- ABI and call/frame behavior: `calls`, `prologue`, `variadic`, `globals`
- scalar and wide operation families: `cast_ops`, `float_ops`, `i128_ops`,
  `f128`
- target-specific extras: `atomics`, `intrinsics`, `inline_asm`,
  `asm_emitter`, `peephole`

The exact order may change during planning, but each execution packet should
stay bounded to one shard or one tightly coupled pair and include its own C
fixture proof.

## Boundaries

- Do not treat markdown deletion or renaming as implementation progress.
- Do not port the old reference shape blindly if it conflicts with the current
  prepared-BIR-to-MIR route from idea 228.
- Do not resurrect deleted broad legacy module-emitter or record-pile
  ownership to make a proof pass.
- Do not downgrade backend cases, mark supported paths unsupported, or accept
  expectation-only progress.
- Do not add named-case shortcuts in C++ that only recognize the proof fixture.
- Do not claim a shard complete until its matching `tests/backend/case/xxxx.c`
  proof is passing through CTest.

## Completion Signal

This idea is complete when every `.md` shard in
`src/backend/mir/aarch64/codegen/` has been converted into active compiled C++,
every converted shard has a corresponding `tests/backend/case/xxxx.c` proof
recorded in the ledger, the relevant backend/MIR CTest selectors pass, and no
markdown artifact remains as the only owner of live AArch64 codegen behavior.

## Reviewer Reject Signals

Reject the route if a shard is marked converted without compiled C++ and a
passing backend C fixture, if the implementation adds fixture-shaped special
cases, if tests are weakened to make conversion appear green, if old legacy
module ownership is restored under new filenames, or if proof only checks that
the file exists rather than that the converted capability works.
