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

## Lifecycle Handoff

Step 6 found that i128 helper-boundary lowering cannot proceed without a
separate prepared/shared authority initiative. That prerequisite was split to
`ideas/closed/248_prepared_i128_runtime_helper_authority.md` and has now been
closed for the supported div/rem helper families.

The prerequisite supplied helper facts for currently supported i128 div/rem
runtime calls:

- source operation identity mapped to helper kind and callee symbol
- low/high argument lane bindings
- low/high result lane bindings for direct-result div/rem helpers
- helper-specific clobber and resource policy
- ABI and register-bank transition facts needed at the helper boundary

This idea can resume at Step 6 for selected AArch64 helper-boundary consumption
of supported div/rem helpers. Float/i128 conversion helper mapping and future
memory-return helper families remain deferred unless a separate prepared/shared
authority route supplies those facts.

## Exhausted Runbook Blocker

Step 8 validated the accepted structured i128 pair-lowering subset, but the
source idea is not closeable while div/rem helper-boundary terminal call
printing still lacks structured helper marshaling and ABI register-binding
authority. The active follow-up prerequisite is now
`ideas/open/249_prepared_i128_helper_marshaling_abi_binding.md`.

That prerequisite must supply the remaining helper-call printing authority for
supported i128 div/rem helper boundaries:

- ABI argument and result register bindings for low/high lanes
- structured moves between prepared carrier lanes and helper ABI registers
- call-clobber and live-value preservation facts needed by helper marshaling
- selected-call operand ownership for terminal `bl <callee>` output
- fail-closed diagnostics for incomplete marshaling, wrong carrier shape, or
  unsupported memory-return/helper-family cases

This idea should resume only after selected AArch64 helper-boundary printing
can consume those facts without target-local fixed-register assumptions,
register-adjacency inference, or opcode-shaped helper synthesis.
