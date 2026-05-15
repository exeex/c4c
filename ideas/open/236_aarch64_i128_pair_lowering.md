# AArch64 I128 Pair Lowering

Status: Open
Created: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

BIR and ABI preparation can name `I128`, and call/return legalization treats
some i128 ABI paths as memory, but current AArch64 codegen has no selected
machine-node family for 128-bit integer pair transport, arithmetic, shifts,
comparisons, or helper-call boundaries. The register allocator currently does
not classify ordinary i128 liveness values into a usable target register
class, so the missing feature spans prepared allocation facts and AArch64 pair
machine-node lowering.

The archived `i128_ops.md` is useful semantic evidence, but its fixed
`x0`/`x1` accumulator pair and local helper-call marshaling are not current
authority.

## Scope

- Establish prepared low/high pair homes or memory-backed i128 carriers before
  AArch64 lowering consumes them.
- Add structured AArch64 machine nodes for i128 pair load/store transport,
  add/sub/bitwise operations, shifts, comparisons, and required runtime helper
  calls.
- Preserve low/high lane ordering and helper-call clobber/resource facts in
  structured records.

## Non-Goals

- Do not create a separate local i128 allocator inside AArch64 codegen.
- Do not infer pair homes from rendered register names.
- Do not lower i128 as scalar i64 or through named proof-case shortcuts.

## Proof Direction

- A value requiring i128 transport has explicit prepared low/high or memory
  carrier facts.
- I128 add/sub and comparisons preserve carry/borrow and signed/unsigned
  high-word behavior.
- Runtime div/rem or float-conversion helper calls consume structured argument,
  result, and clobber facts.

## Lifecycle Blocker

Step 6 found that i128 helper-boundary lowering cannot proceed without a
separate prepared/shared authority initiative. The active prerequisite is now
`ideas/open/248_prepared_i128_runtime_helper_authority.md`.

That prerequisite must supply helper facts for i128 operations that require
runtime calls:

- source operation identity mapped to helper kind and callee symbol
- low/high argument lane bindings
- low/high result lane bindings or memory-return ownership
- helper-specific clobber and resource policy
- ABI and register-bank transition facts needed at the helper boundary

This idea should resume at Step 6 only after those facts exist as prepared or
shared authority. Step 7 printer work must not proceed while helper boundary
records would require AArch64 target lowering to choose helper families,
callee names, fixed registers, lane bindings, or clobber policy locally.
