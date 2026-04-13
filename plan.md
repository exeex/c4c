# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Activated from: fresh activation on 2026-04-13 after closing the x86-specific recovery idea and resetting the route onto a backend-wide BIR-first reboot

## Purpose

Replace the old direct-route backend shape with one truthful shared pipeline:

`LIR -> semantic BIR -> prepare/legalize -> target backend`

## Goal

Make BIR the real backend spine so x86, aarch64, and riscv64 no longer depend
on raw-LIR fallback paths or testcase-shaped recovery hooks.

## Core Rule

Do not reintroduce `try_emit_*`, `try_lower_*`, direct-dispatch ladders,
rendered-text case matchers, or equivalent workaround seams under new names.

## Read First

- `ideas/open/46_backend_reboot_bir_spine.md`
- `src/backend/backend.cpp`
- `src/backend/bir.hpp`
- `src/backend/bir_printer.cpp`
- `src/backend/bir_validate.cpp`
- `src/backend/lowering/lir_to_bir.hpp`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/prepare/prepare.hpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/prepare/legalize.cpp`

## Current Targets

- `src/backend/backend.cpp`
- `src/backend/bir.hpp`
- `src/backend/bir_printer.cpp`
- `src/backend/bir_validate.cpp`
- `src/backend/lowering/lir_to_bir.hpp`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/prepare/prepare.hpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/prepare/legalize.cpp`

## Non-Goals

- do not rebuild target asm emission before the shared BIR/prepare spine is
  trustworthy
- do not claim c-testsuite recovery from direct route deletion alone
- do not patch individual case families with route-specific detection logic
- do not push target legality checks back into semantic `lir_to_bir`

## Working Model

- semantic BIR must preserve compiler meaning, not LLVM text artifacts
- target legality belongs in `prepare/legalize`, not in `lir_to_bir`
- structural validation may stay in BIR, but type legality should not block
  semantic lowering
- the first milestone is not "whole backend works"; it is "BIR is credible
  enough that more lanes can be added without direct escapes"

## Ordered Steps

### Step 1: Make scalar semantic BIR credible

Goal: establish a clean semantic lowering lane for params, compare, branch,
select, and return.

Primary target:

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/bir.hpp`
- `src/backend/bir_printer.cpp`
- `src/backend/bir_validate.cpp`

Concrete actions:

- lower scalar params and returns into BIR
- canonicalize compare-driven `cond_br`
- canonicalize `diamond + phi` ternary shapes into `bir.select`
- keep semantic `i1` in BIR and leave widening to `prepare`

Completion check:

- `branch_if_eq.c` lowers to BIR without LLVM-ish bool bridge noise
- `single_param_select_eq.c` lowers to BIR as semantic `select`
- nearby scalar compare/select cases can be reasoned about through BIR text

### Step 2: Add semantic memory/global/call lanes

Goal: expand BIR beyond toy scalar control-flow functions.

Primary target:

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/bir.hpp`

Concrete actions:

- lower local slots, loads, stores, and address forms
- lower globals and string constants into honest BIR module state
- lower direct calls and the minimal shared call metadata needed downstream

Completion check:

- simple local/global/call backend-route cases lower to BIR
- module lowering no longer rejects whole modules just because they contain
  globals or calls

### Step 3: Make `prepare` own target legality

Goal: move target-specific shaping out of semantic lowering.

Primary target:

- `src/backend/prepare/legalize.cpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/backend.cpp`

Concrete actions:

- keep semantic BIR target-neutral where possible
- legalize `i1` and other target-specific value forms in `prepare`
- define the prepared-BIR contract that target backends are expected to ingest

Completion check:

- semantic BIR can still express `i1`
- prepare rewrites target-specific legality for x86/i686/aarch64/riscv64
- backend driver clearly routes BIR through prepare before target codegen

### Step 4: Build real prepare phases after legalization

Goal: stop treating `prepare` as a stub layer.

Primary target:

- `src/backend/prepare/stack_layout.cpp`
- `src/backend/prepare/liveness.cpp`
- `src/backend/prepare/regalloc.cpp`

Concrete actions:

- define stack objects and frame layout inputs
- add liveness data collection
- add initial register-allocation plumbing over prepared BIR

Completion check:

- prepare records real phase output instead of only notes
- target backends can assume prepared-BIR shape instead of reconstructing it

### Step 5: Reconnect target backends to prepared BIR only

Goal: make x86, aarch64, and riscv64 consume the new shared spine.

Primary target:

- `src/backend/backend.cpp`
- target backend ingestion entry points under `src/backend/*/codegen/`

Concrete actions:

- define target ingestion contracts in terms of prepared BIR
- reject raw/direct LIR fallback paths
- expand proving only after prepared-BIR ingestion is real

Completion check:

- backend routing no longer depends on direct LIR escapes
- target recovery is explained by shared BIR/prepare capability growth

## Validation Ladder

- `cmake --build build -j2`
- focused backend-route observation using `./build/c4cll --codegen asm --target <triple> <case>.c -o <out>`
- escalate to matching internal/c-testsuite subsets only after the BIR/prepare
  lane for that capability is real
