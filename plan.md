# HIR -> LIR Split Plan

## Purpose

This document is for an execution-oriented AI agent.

Read it as a runbook.
Do not redesign the compiler.
Do not try to build the perfect LIR in one diff.
Do not remove the legacy LLVM path until the LIR path is clearly stable.


## One-Sentence Goal

Finish the split from:

- HIR -> mixed lowering+printing

to:

- HIR -> structured LIR -> LLVM printer

while keeping the existing LLVM output path working.


## Current Reality

This repo is already partway through the split.

These pieces already exist:

1. `--codegen legacy|lir|compare` CLI support in
   [c4cll.cpp](/workspaces/c4c/src/apps/c4cll.cpp)
2. legacy entrypoint in
   [llvm_codegen.cpp](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
3. LIR data model in
   [ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)
4. LIR lowering entrypoint in
   [hir_to_lir.hpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.hpp)
   and
   [hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp)
5. LIR printer in
   [lir_printer.hpp](/workspaces/c4c/src/codegen/lir/lir_printer.hpp)
   and
   [lir_printer.cpp](/workspaces/c4c/src/codegen/lir/lir_printer.cpp)
6. shared lowering context in
   [fn_lowering_ctx.hpp](/workspaces/c4c/src/codegen/shared/fn_lowering_ctx.hpp)

What is still wrong:

- `hir_to_lir.cpp` still depends heavily on legacy `HirEmitter`
- parts of LIR still carry raw LLVM-text fallback ops
- the split exists structurally, but too much real lowering logic is still
  inherited from the legacy emitter path
- printer independence is incomplete if LIR lowering still needs LLVM text
  helpers to think


## Files You Will Touch

Primary files:

1. [hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp)
- remove dependence on legacy emitter behavior
- make LIR lowering own more structure directly

2. [ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)
- extend LIR nodes only when a concrete missing operation forces it

3. [lir_printer.cpp](/workspaces/c4c/src/codegen/lir/lir_printer.cpp)
- keep LLVM syntax rendering here
- do not move semantic lowering into this file

4. [llvm_codegen.cpp](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- keep this as orchestration only

5. [hir_emitter.hpp](/workspaces/c4c/src/codegen/llvm/hir_emitter.hpp)
- only touch when extracting logic or reducing legacy ownership

Do not start in:

- frontend HIR files
- parser files
- tests unrelated to codegen


## Current Entry Points

Before editing, read these exact points:

1. legacy orchestration:
- [llvm_codegen.cpp](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)

2. current LIR lowering:
- [hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp)

3. current LIR model:
- [ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)

4. current LIR printer:
- [lir_printer.cpp](/workspaces/c4c/src/codegen/lir/lir_printer.cpp)

5. shared function lowering state:
- [fn_lowering_ctx.hpp](/workspaces/c4c/src/codegen/shared/fn_lowering_ctx.hpp)

6. legacy combined lowering+printing implementation:
- [hir_emitter.hpp](/workspaces/c4c/src/codegen/llvm/hir_emitter.hpp)


## What “Done” Means

This track is complete when all of these are true:

1. `llvm_codegen.cpp` is a thin orchestration layer
2. `hir_to_lir` can lower without relying on `HirEmitter` as the semantic owner
3. `lir_printer` only prints; it does not repair or invent semantics
4. LIR is structured enough that the main path does not depend on raw
   `vector<string>`-style fallback lowering
5. compare mode stays green for the focused codegen corpus


## Rules For The Agent

Follow these rules while implementing:

1. Make one extraction step per diff.
2. Preserve output behavior before generalizing.
3. If a decision is LLVM syntax only, keep it in the printer.
4. If a decision is about evaluation, control flow, storage, or operand
   meaning, move it into `hir_to_lir`.
5. Only add a new LIR op when a concrete legacy special case forces it.
6. Do not delete the legacy path until compare mode is trustworthy.


## Current Problem To Solve

The real problem is not “we lack LIR files”.

The real problem is:

- the LIR path still inherits too much logic from the legacy emitter
- some operations are still represented as raw LLVM text fallback nodes
- ownership boundaries are not clean enough yet

Your job is to complete the ownership split, not to rename files.


## Implementation Order

Do the work in this order.
Do not skip ahead.


## Step 1. Audit Current Legacy Dependencies

Goal:

- identify exactly where `hir_to_lir` still relies on legacy `HirEmitter`

Do this:

1. open
   [hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp)
2. list every use of:
   - `HirEmitter`
   - legacy LLVM helper namespaces
   - raw LLVM text fallback instructions / terminators
3. group each dependency into one of:
   - orchestration only
   - type/helper reuse only
   - semantic lowering dependency
   - printer-only formatting dependency accidentally used during lowering

Completion check:

- you can point to the exact remaining semantic dependencies that keep the
  split incomplete


## Step 2. Remove One Semantic Dependency At A Time

Goal:

- each diff should eliminate one meaningful legacy ownership dependency

Do this:

1. choose one dependency from Step 1
2. move the semantic part into `hir_to_lir.cpp` or a shared non-printer helper
3. keep LLVM text formatting in `lir_printer.cpp`
4. keep behavior the same

Good candidates:

- per-function lowering state ownership
- intrinsic requirement tracking
- string pool ownership
- control-flow/terminator construction
- local/global/function dedup bookkeeping

Do not do in one diff:

- all special cases at once
- full `HirEmitter` deletion


## Step 3. Replace Raw LLVM-Text Fallback LIR Where It Hurts Most

Goal:

- reduce the parts of LIR that are still “structured shell around LLVM text”

Read first:

- [ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)

Do this:

1. find the most semantically important raw fallback instruction/terminator
2. add one explicit LIR op for it
3. lower to that new op in `hir_to_lir.cpp`
4. print it in `lir_printer.cpp`
5. verify compare mode still matches

Prioritize these before cosmetic ops:

- control-flow terminators
- loads/stores
- calls
- GEP/member/index address calculations
- special storage ops for VLAs / memcpy / varargs when they still leak semantics


## Step 4. Keep Printer Pure

Goal:

- printer must render complete LIR, not fix broken LIR

Do this:

1. inspect new or existing branches in
   [lir_printer.cpp](/workspaces/c4c/src/codegen/lir/lir_printer.cpp)
2. if a branch is deciding semantics rather than spelling syntax, move that
   decision back into `hir_to_lir.cpp`

Allowed in printer:

- `%tN` naming
- `@global` quoting
- LLVM declaration syntax
- LLVM intrinsic spelling
- final textual formatting

Not allowed in printer:

- inventing missing terminators
- deciding storage semantics
- repairing malformed operand meaning
- reconstructing control flow that lowering failed to encode


## Step 5. Reduce `llvm_codegen.cpp` To Orchestration Only

Goal:

- top-level LLVM codegen should just choose path and orchestrate

Target file:

- [llvm_codegen.cpp](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)

Do this:

1. keep `emit_legacy(...)`
2. keep `emit_lir(...)`
3. avoid adding new lowering logic here
4. if this file needs knowledge of HIR lowering internals, move that logic
   downward instead

Completion check:

- this file reads like a path switcher, not like a backend implementation


## Step 6. Only Then Consider Shrinking `HirEmitter`

Goal:

- reduce or remove legacy code only after the LIR path owns the same behavior

Do this:

1. verify compare mode on a focused corpus
2. verify the LIR path no longer needs one legacy subsystem
3. then shrink that subsystem from `HirEmitter`

Do not delete first and repair later.


## How To Decide Whether A Helper Belongs In Lowering Or Printing

Use this rule:

1. If it answers “what operation is this?”:
- lowering

2. If it answers “what blocks/operands/results exist?”:
- lowering

3. If it answers “how should LLVM text spell this?”:
- printer

4. If it exists only because the old emitter built strings inline:
- probably split it


## Special Cases To Migrate Late

Leave these for later slices unless they are your explicit target:

- bitfields
- VLAs
- varargs helpers
- inline asm
- computed goto / blockaddress

These are high-risk because current code likely mixes semantics and LLVM text
most tightly here.


## Minimum Validation Strategy

After each meaningful diff:

1. build `c4cll`
2. run the focused codegen corpus under:
   - `--codegen legacy`
   - `--codegen lir`
   - `--codegen compare`
3. if compare differs, inspect whether it is harmless textual churn or a real
   semantic mismatch

Suggested focus corpus:

- simple arithmetic/functions
- structs/unions/bitfields
- arrays and pointer arithmetic
- control flow and switch
- globals and initializers
- varargs
- inline asm
- computed goto if supported
- VLA cases


## Good First Diff

A good first diff does exactly this:

1. audits one real remaining legacy dependency
2. moves that ownership into `hir_to_lir`
3. keeps printed LLVM output behavior stable
4. leaves broader LIR redesign alone


## Good Second Diff

A good second diff does exactly this:

1. replaces one raw LLVM-text fallback LIR op with a typed LIR op
2. updates the printer for that op
3. proves compare mode still works


## Abort Conditions

Stop and reassess if either of these happens:

1. you need a frontend HIR redesign just to continue the split
2. you are adding many new LIR ops without deleting any real legacy dependency

If that happens:

- make the smallest safe extraction
- update this plan
- do not silently expand the project scope


## Short Version

If you only remember one sequence, remember this:

1. audit where `hir_to_lir` still depends on `HirEmitter`
2. remove one semantic dependency at a time
3. replace the worst raw LLVM-text fallback ops with typed LIR ops
4. keep the printer responsible only for syntax
5. keep compare mode green while shrinking the legacy path
