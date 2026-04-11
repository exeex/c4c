# x86_64 Translated Codegen Integration

Status: Active
Source Idea: ideas/open/43_x86_64_peephole_pipeline_completion.md
Activated from: lifecycle reset on 2026-04-11

## Purpose

Turn the translated x86 codegen owner surface into the real backend path instead
of continuing to extend the legacy `emit.cpp` matcher surface with more bounded
test-only seams.

## Goal

Expose the minimum shared x86 codegen surface needed to compile translated owner
units, switch one real owner cluster into the active backend path, and leave
test expansion limited to the narrow proof needed for each ownership transfer.

## Core Rules

- Follow the source idea's translated-owner integration scope; do not mutate
  this into shared-BIR cleanup from idea 44.
- Do not spend iterations on testcase-matrix growth unless a missing test is
  required to prove the current ownership slice.
- Prefer shared-header and owner-surface work over adding new
  `emit.cpp`-local logic.
- If execution uncovers adjacent but non-blocking backend gaps, record them in
  the source idea instead of widening this runbook.
- Keep validation backend-focused: targeted tests per slice, then one broad
  regression guard after a meaningful owner-path change.

## Read First

- `ideas/open/43_x86_64_peephole_pipeline_completion.md`
- `src/backend/x86/codegen/x86_codegen.hpp`
- `src/backend/x86/codegen/emit.cpp`
- `src/backend/x86/codegen/{calls,returns,prologue,globals}.cpp`
- `ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs`

## Scope

- shared x86 codegen declarations needed by translated owner files
- translated top-level units already present in `src/backend/x86/codegen/`
- active owner transfer away from legacy `emit.cpp`
- focused backend regression coverage that proves the transferred path runs

## Non-Goals

- no family-by-family direct-call testcase expansion as a primary work queue
- no unrelated frontend, parser, LLVM IR, or runtime work
- no broad ABI cleanup unless the active translated-owner slice requires it
- no silent absorption of idea 44 or RV64 work

## Execution Steps

### Step 1: Rebuild the dependency audit around owner blockers

Goal: make the next ownership slice explicit in terms of missing shared types,
helpers, and state on the current C++ surface.

Primary targets:

- `src/backend/x86/codegen/x86_codegen.hpp`
- `src/backend/x86/codegen/{calls,returns,prologue}.cpp`
- `ref/claudes-c-compiler/src/backend/x86/codegen/emit.rs`

Actions:

- verify which translated owner files are blocked by missing public
  declarations rather than missing CMake wiring
- record the smallest compilable owner cluster that reduces real `emit.cpp`
  ownership
- identify the minimum test needed to prove that cluster once connected

Completion check:

- the next owner cluster is explicit
- the missing shared declarations are explicit
- the planned proof tests are bounded

### Step 2: Surface the minimum shared x86 codegen contract

Goal: expose enough shared declarations in `x86_codegen.hpp` for the chosen
translated owner cluster to compile and link.

Actions:

- move required enums, structs, helpers, and `X86Codegen` declarations out of
  hidden local surfaces
- keep the surfaced API limited to the active owner cluster
- wire the minimum required translated files into the build

Completion check:

- the chosen translated owner cluster compiles in the main build
- no new `emit.cpp`-local seam is added to avoid the header work

### Step 3: Transfer one real owner path out of `emit.cpp`

Goal: replace one legacy owner slice with the translated owner path as runtime
behavior, not just symbol coverage.

Actions:

- route the active backend path through the chosen translated owner cluster
- delete or bypass the corresponding legacy `emit.cpp` ownership for that slice
- add only the narrowest backend regression needed to prove the switched path

Completion check:

- runtime ownership for the chosen slice no longer lives in legacy `emit.cpp`
- targeted backend tests prove the translated path is active

### Step 4: Validate and park residual work explicitly

Goal: end the iteration with a stable execution state instead of an open-ended
test backlog.

Actions:

- run the focused backend validation for the switched slice
- run one broad regression-guard pass after the meaningful owner-path change
- record remaining disconnected translated units or deferred follow-ons in the
  source idea or next `todo.md` state

Completion check:

- validation results are recorded
- remaining work is framed as explicit owner-path follow-ons, not generic test
  expansion
- the next iteration can resume from one bounded technical blocker
