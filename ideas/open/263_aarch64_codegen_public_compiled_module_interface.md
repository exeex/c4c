# AArch64 Codegen Public Compiled-Module Interface

Status: Open
Created: 2026-05-17

## Intent

Make the AArch64 codegen public handoff expose compiled artifacts through the
`c4c::backend::aarch64::codegen` API rather than through misleading
`aarch64::module` names.

The external codegen surface should be
`src/backend/mir/aarch64/codegen/codegen.hpp`. Public compiled artifact names
should be clear codegen names such as `codegen::CompiledModule`,
`codegen::CompileResult`, and `codegen::compile_prepared_module(...)`, or
equivalent names that make the compiled-codegen boundary explicit.

## Why This Exists

`module::BuildResult` and `module::Module` are wrongly layered for the public
handoff from AArch64 codegen. The names make the compiled artifact look like a
generic module subsystem concern rather than the result of the AArch64 codegen
pipeline. Future printer, assembler, and encoder work should consume a
compiled module structure owned by codegen, not by `aarch64::module` or
`aarch64::assembly`.

## In Scope

- Add `src/backend/mir/aarch64/codegen/codegen.hpp` as the public external
  AArch64 codegen header.
- Move or alias the public handoff types from
  `aarch64::module::{Module, BuildResult}` to
  `aarch64::codegen::{CompiledModule, CompileResult}` or equivalent clear
  names.
- Provide a public compile entry point such as
  `codegen::compile_prepared_module(...)` when that matches the existing
  pipeline shape.
- Update API-facing code, backend callers, tests, and docs or contracts as
  needed to use the codegen public interface.
- Keep lower-level lowering contexts, diagnostic helpers, and internal module
  implementation details where safest for now.
- Preserve compatibility wrappers only when they are needed for a staged
  migration, while ensuring the advertised public route stops presenting
  `module::Module` and `module::BuildResult` as the compiled result.

## Out of Scope

- Semantic lowering changes.
- Broad namespace purges unrelated to the public compiled-module handoff.
- Moving printer, assembler, or encoder ownership in this idea beyond making
  them able to consume the codegen-owned compiled module contract later.
- Renaming lower-level implementation helpers solely for aesthetic consistency.
- Rewriting tests or expectations to hide unchanged public API exposure.

## Completion Criteria

- `src/backend/mir/aarch64/codegen/codegen.hpp` exists and is the documented
  public AArch64 codegen header.
- Public compiled artifacts are exposed under
  `c4c::backend::aarch64::codegen` with clear names such as
  `CompiledModule` and `CompileResult`, or equivalent names.
- Public callers and tests use the codegen public interface instead of
  advertising `aarch64::module::{Module, BuildResult}` as the compiled result.
- Any temporary compatibility wrappers are clearly non-primary and do not keep
  the old `module::` names as the public route.
- Behavior is preserved: no semantic lowering changes are introduced.
- Proof covers AArch64 backend/MIR tests and public assembly smoke, with
  broader validation if the public API blast radius requires it.

## Reviewer Reject Signals

Reject the route or slice if it:

- changes lowering semantics, instruction selection, emitted assembly, or test
  expectations while claiming this interface cleanup as behavior-preserving;
- only adds typedefs or helper renames while the advertised public API still
  directs callers to `aarch64::module::{Module, BuildResult}` for compiled
  codegen artifacts;
- moves the compiled artifact under `aarch64::assembly` or another
  non-codegen namespace that obscures the codegen handoff;
- performs a broad namespace purge of internal lowering contexts or diagnostics
  outside the public compiled-module interface cleanup;
- weakens public API tests, downgrades AArch64 backend coverage, or removes asm
  smoke proof without explicit user approval;
- adds testcase-shaped shortcuts or named-case-only behavior to prove the
  refactor instead of preserving the existing pipeline behavior;
- leaves future printer, assembler, or encoder consumers with no clear
  codegen-owned compiled module contract to consume.
