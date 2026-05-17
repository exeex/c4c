# AArch64 Codegen Public Compiled-Module Interface Runbook

Status: Active
Source Idea: ideas/open/263_aarch64_codegen_public_compiled_module_interface.md

## Purpose

Clean up the public AArch64 codegen handoff so compiled artifacts are exposed
through `c4c::backend::aarch64::codegen`, not through misleading
`aarch64::module` names.

Goal: introduce `src/backend/mir/aarch64/codegen/codegen.hpp` as the external
codegen API and make the public compiled-module result use clear codegen names
such as `CompiledModule`, `CompileResult`, and
`compile_prepared_module(...)`, or equivalent local names.

Core Rule: this is a behavior-preserving interface and naming cleanup, not a
semantic lowering change or broad namespace purge.

## Read First

- `ideas/open/263_aarch64_codegen_public_compiled_module_interface.md`
- Existing AArch64 codegen public and internal headers under
  `src/backend/mir/aarch64/codegen/`
- Existing `aarch64::module::Module` and `aarch64::module::BuildResult`
  definitions and callers
- API-facing backend entry points that expose or consume compiled AArch64
  artifacts
- Tests or docs that name the public AArch64 compiled-module contract

## Current Scope

- Add `src/backend/mir/aarch64/codegen/codegen.hpp`.
- Make `c4c::backend::aarch64::codegen` the public namespace for compiled
  AArch64 codegen artifacts.
- Move or alias public handoff names from
  `aarch64::module::{Module, BuildResult}` to codegen-owned names.
- Update API-facing callers, backend callers, tests, and docs or contracts
  needed to use the codegen public interface.

## Non-Goals

- Do not change lowering semantics, instruction selection, emitted assembly, or
  diagnostics behavior.
- Do not require a broad purge of all lower-level `module` implementation
  helpers, lowering contexts, or diagnostics in one step.
- Do not move printer, assembler, or encoder ownership beyond establishing the
  compiled module contract they can consume later.
- Do not rewrite tests or expectations to hide unchanged public API exposure.
- Do not rename internal helpers solely for cosmetic namespace consistency.

## Working Model

The public handoff from AArch64 codegen is the compiled module result. That
result belongs under `c4c::backend::aarch64::codegen` and should be reachable
from `codegen.hpp`.

Lower-level implementation can keep existing internal structure while the
public surface migrates. Compatibility wrappers may exist temporarily if they
reduce blast radius, but the primary route advertised to public callers must be
the codegen interface.

## Execution Rules

- Preserve behavior first; keep each packet compile-proven.
- Prefer local AArch64 naming and header patterns over a new abstraction style.
- Keep compatibility wrappers small, clearly secondary, and removable.
- Update tests and docs/contracts with the interface migration, not as a
  substitute for it.
- Keep source intent in the linked idea stable unless the durable goal changes.
- Update `todo.md` with packet progress and proof; do not churn this runbook
  for routine implementation notes.
- Treat typedef-only progress, expectation-only movement, and broad namespace
  churn as route failures.
- Before closure, prove AArch64 backend/MIR coverage and public asm smoke; run
  broader validation if API-facing blast radius requires it.

## Ordered Steps

### Step 1: Inventory the current compiled-module public surface

Goal: identify every definition and public caller of the current AArch64
compiled-module handoff.

Primary targets:
- Current headers/sources defining `aarch64::module::Module`
- Current headers/sources defining `aarch64::module::BuildResult`
- API-facing backend entry points and tests that include or name those types

Actions:
- Inspect where `module::Module` and `module::BuildResult` are defined,
  included, returned, and documented.
- Separate true public handoff usage from internal implementation details that
  can safely remain in place for now.
- Identify the minimal first packet for adding `codegen.hpp` and public
  codegen-owned names.
- Record the first implementation packet and proof command in `todo.md`.

Completion check:
- The executor can name the current public surfaces, the internal surfaces to
  leave alone, and the first compile/test proof before editing code.

### Step 2: Add the public codegen header and names

Goal: introduce the public AArch64 codegen API surface without changing
compiled output behavior.

Primary targets:
- `src/backend/mir/aarch64/codegen/codegen.hpp`
- Existing compiled-module definition header/source
- Build or install/export plumbing for public headers, if explicit

Actions:
- Add `codegen.hpp` with namespace layout
  `c4c::backend::aarch64::codegen`.
- Expose public compiled artifact names such as `CompiledModule` and
  `CompileResult`, or equivalent clear names.
- Add or route a public compile entry point such as
  `compile_prepared_module(...)` if that matches the current pipeline shape.
- Keep old `module::` wrappers only if needed for staged migration.
- Run a narrow build proof after the header and names compile.

Completion check:
- `codegen.hpp` compiles, public codegen-owned names exist, and no caller
  behavior has changed.

### Step 3: Migrate API-facing and backend callers

Goal: make public and backend-facing routes use the codegen public interface.

Primary targets:
- API-facing backend sources
- AArch64 backend callers
- Tests that include or spell the compiled-module handoff

Actions:
- Replace public includes and type spellings with `codegen.hpp` and
  codegen-owned names.
- Update backend callers to prefer `codegen::CompiledModule`,
  `codegen::CompileResult`, and the public compile entry point.
- Keep internal lower-level module details intact when changing them would
  broaden the slice.
- Preserve wrappers only where a staged transition needs them.
- Compile after each meaningful caller migration if the blast radius is broad.

Completion check:
- API-facing and backend callers no longer advertise
  `module::Module`/`module::BuildResult` as the public compiled result.

### Step 4: Update tests, docs, and compatibility boundaries

Goal: align contracts with the new codegen public interface and make any
temporary old-route compatibility explicit.

Primary targets:
- Public API tests
- AArch64 backend/MIR tests
- Docs or contract files that mention the compiled-module interface
- Temporary compatibility wrappers, if any

Actions:
- Update tests and docs to spell the public route through `aarch64::codegen`.
- Add or adjust smoke coverage that proves public asm generation still works
  through the new compiled-module API.
- Mark compatibility wrappers as secondary or internal where local conventions
  allow it.
- Do not weaken coverage or rewrite expected assembly unless a behavior change
  is explicitly approved elsewhere.

Completion check:
- Tests and docs/contracts advertise the codegen interface, and any remaining
  `module::` compatibility is visibly non-primary.

### Step 5: Final validation and cleanup readiness

Goal: prove the interface cleanup preserved behavior and is ready for closure.

Actions:
- Run the supervisor-selected AArch64 backend/MIR proof.
- Run public assembly smoke through the new interface.
- Escalate to broader backend or full-suite validation if public API changes
  cross more than AArch64 codegen consumers.
- Confirm no lowering semantics, emitted assembly expectations, or diagnostics
  behavior changed.
- Confirm the diff is limited to the public codegen interface cleanup and
  required caller/test/doc updates.

Completion check:
- The supervisor has enough proof to request plan-owner closure review for the
  linked source idea.
