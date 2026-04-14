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
- `src/codegen/lir/ir.hpp`
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

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/bir.hpp`
- `src/backend/bir_printer.cpp`
- `src/backend/bir_validate.cpp`
- `src/backend/prepare/legalize.cpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/backend.cpp`

## Non-Goals

- do not rebuild target asm emission before the shared BIR/prepare spine is
  trustworthy
- do not claim c-testsuite recovery from direct route deletion alone
- do not patch individual case families with route-specific detection logic
- do not push target legality checks back into semantic `lir_to_bir`
- do not let packet selection drift into testcase names when the real missing
  unit is a semantic capability family

## Working Model

- semantic BIR must preserve compiler meaning, not LLVM text artifacts
- target legality belongs in `prepare/legalize`, not in `lir_to_bir`
- structural validation may stay in BIR, but type legality should not block
  semantic lowering
- executor packets should be chosen from the ordered capability backlog below,
  not rediscovered from ad hoc testcase hunting

## Completed Capability Baseline

These lanes are already landed and should remain green while new families are
added:

- scalar compare / branch / return
- single-parameter `select` lowering from `diamond + phi`
- minimal function signature and direct scalar param handling
- local scalar slots, loads, and stores
- local arrays with constant indexing
- direct global calls with minimal typed args
- scalar globals with load and store

## Ordered Capability Backlog

Choose future executor packets from this list in order unless route quality or
review justifies a re-sequencing.

### 1. Generalize CFG merge and `phi`

Goal: stop treating `phi` as a ternary-only special lane.

Primary target:

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/bir.hpp`

Concrete actions:

- widen beyond `diamond + phi -> bir.select`
- support ordinary block-merge `phi` forms without testcase-shaped CFG probes
- define when semantic BIR keeps `phi`-like merge structure versus lowering to
  `select`
- make later lanes consume already-lowered merge semantics:
  call lowering must use shared merged values, not carry its own private
  `phi`/CFG reconstruction rules

Completion check:

- non-trivial merge shapes no longer require raw-LIR fallback
- `phi` handling is explained by CFG semantics, not by one named case family

### 2. Harden params and function signatures

Goal: make function entry and return contracts reliable before broader calls or
ABI shaping.

Primary target:

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/lowering/call_decode.cpp`

Concrete actions:

- do not use the existing `backend_case` `two_arg_*` route stems as
  printed-BIR proof under the current backend contract:
  once lowering succeeds, `src/backend/backend.cpp` legitimately prepares BIR
  and emits target asm for those helpers
- treat helper entry lowering, direct-call argument materialization, and return
  propagation as one semantic signature lane in
  `src/backend/lowering/lir_to_bir_module.cpp` /
  `src/backend/lowering/call_decode.cpp`, but keep the `two_arg_*` helper
  family as runtime/regression sentinels rather than the primary proof anchor
- use `param_slot`, `param_member_array`, and
  `nested_param_member_array` as the first honest backlog-item-2 proving
  surface because they exercise parameter handling by behavior rather than by
  output-mode choice
- if direct observation of signature-lowering success still needs more than
  runtime behavior, allow one narrow harness packet that can observe semantic
  BIR coverage without regressing supported prepared-BIR emission
- keep semantic signature lowering separate from target ABI legalization
- do not count native-asm output on trivial helpers or frontend-promotion
  expectation mismatches as proof of semantic signature progress

Completion check:

- parameter-slot and simple aggregate-param cases are proven by honest
  backend-case behavior or approved harness observation, not by forcing
  supported prepared-BIR emission to print BIR
- scalar direct helper/runtime sentinels remain consistent with the same
  signature-lowering rule instead of requiring named-case branches

### 3. Broaden local memory and address formation

Goal: move from scalar locals and constant-index local arrays to general local
address semantics.

Primary target:

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/bir.hpp`

Concrete actions:

- extend `alloca` / local-slot lowering beyond scalar and fixed tiny array
  shapes
- lower broader `gep` and address forms for locals
- support pointer-valued locals and round-tripped local addresses where the
  semantics are still target-neutral

Completion check:

- local memory lowering no longer stops at constant-index toy arrays

### 4. Broaden global data and addressed globals

Goal: make module-level state real beyond scalar globals.

Primary target:

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/bir.hpp`

Concrete actions:

- lower global arrays and string-backed globals
- support extern global declarations and addressed global reads
- support pointer-valued globals and basic global-address arithmetic only where
  the semantics are still honest BIR

Completion check:

- addressed global data no longer forces LLVM-text fallback for simple array or
  string-backed reads

### 5. Expand call lowering beyond minimal direct calls

Goal: make calls a shared semantic lane instead of a tiny direct-call subset.

Primary target:

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/lowering/call_decode.cpp`
- `src/backend/bir.hpp`

Concrete actions:

- support richer direct-call signatures
- support indirect calls and callee pointer forms
- treat general `phi`/CFG merge handling as an input contract to this family:
  call lowering should consume merge-preserved values that semantic BIR
  already knows how to represent, rather than extending call-specific merge
  workarounds
- treat callee provenance as a semantic family:
  globals, loaded/stored function pointers, and merge-preserved callee values
  should be lowered by meaning rather than rediscovered by wider arg-count
  wrappers
- carry the minimal shared metadata needed for later ABI shaping
- do not treat "next wider signature" or longer rendered-asm snippet ladders as
  executor packet selection; backlog item 5 progress must come from real
  lowering/backend capability growth or from an explicit route checkpoint

Completion check:

- semantic BIR can represent the common direct and indirect call forms present
  in LIR without dropping back to raw LIR text
- call-lane progress is explained by new semantic callee/signature families,
  not by extending testcase-shaped arg-count probes

### 6. Add intrinsic and runtime operation lowering

Goal: stop treating runtime-facing LIR operations as a reason to bypass BIR.

Primary target:

- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/bir.hpp`

Concrete actions:

- add semantic lowering for the first runtime and intrinsic families:
  `memcpy`, `memset`, `va_*`, `stacksave`, `stackrestore`, `abs`, and inline
  asm placeholder routing where applicable
- keep this layer semantic; do not smuggle target legalization into the
  intrinsic lowering itself

Completion check:

- intrinsic/runtime-heavy functions can at least enter semantic BIR instead of
  forcing route escape

### 7. Define semantic BIR memory/value coverage boundaries

Goal: make unsupported surface explicit by semantics instead of accidental
packet history.

Primary target:

- `src/backend/bir.hpp`
- `src/backend/lowering/lir_to_bir.hpp`
- `src/backend/lowering/lir_to_bir_module.cpp`

Concrete actions:

- decide which aggregate, vector, and pointer-difference forms belong in
  semantic BIR now versus later
- keep the unsupported set organized by semantic family, not by testcase name
- ensure unsupported routes fail honestly instead of regressing into hidden
  direct emission seams

Completion check:

- remaining gaps are traceable by capability family from the runbook itself

### 8. Make `prepare/legalize` the owner of target legality

Goal: move from semantic BIR coverage to prepared-BIR contracts.

Primary target:

- `src/backend/prepare/legalize.cpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/backend.cpp`

Concrete actions:

- preserve target-neutral semantic BIR where possible
- legalize `i1` and other target-specific value forms in `prepare`
- define prepared-BIR invariants expected by x86/i686/aarch64/riscv64

Completion check:

- backend driver clearly routes semantic BIR through prepare before target
  codegen
- target legality lives in prepare rather than creeping back into lowering

### 9. Build real prepare phases after legalization

Goal: stop treating `prepare` as a shell.

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

### 10. Reconnect target backends to prepared BIR only

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
- focused backend-route observation using
  `./build/c4cll --codegen asm --target <triple> <case>.c -o <out>`
- escalate to matching internal/c-testsuite subsets only after the BIR/prepare
  lane for that capability is real
