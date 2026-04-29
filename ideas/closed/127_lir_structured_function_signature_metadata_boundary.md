# LIR Structured Function Signature Metadata Boundary

Status: Closed
Created: 2026-04-29
Closed: 2026-04-29

Parent Ideas:
- [125_lir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/125_lir_legacy_string_lookup_removal_convergence.md)
- [126_bir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/126_bir_legacy_string_lookup_removal_convergence.md)

## Goal

Stop treating `LirFunction::signature_text` as a cross-IR semantic source for
function signature facts. Backend/BIR consumers and LIR verifier semantic
checks should get return, parameter, variadic, aggregate, and ABI-relevant
signature metadata from structured LIR carriers produced by HIR-to-LIR, not by
parsing the final LLVM header string.

## Why This Idea Exists

`signature_text` is legitimate in the LIR printer because the printer's job is
to render final LLVM text. It is also reasonable for diagnostics, dumps, and
final-render consistency checks. It is not a good cross-IR interface for
backend lowering or verifier semantic facts.

The current LIR model already has partial structured mirrors such as
`LirFunction::return_type`, `LirFunction::params`,
`signature_return_type_ref`, and `signature_param_type_refs`. However,
BIR/backend lowering still parses `signature_text` to recover function
signature details needed for semantic lowering and ABI decisions. The LIR
verifier also parses the rendered header while checking the existing signature
mirrors. These paths should treat structured metadata as the primary fact
source; `signature_text` should only be compared as final emitted spelling or
used for fallback diagnostics. Otherwise final spelling remains a hidden
semantic authority across the LIR -> BIR/backend module boundary.

This idea owns the cleanup of that legacy string path by either completing the
existing structured signature carriers or adding a narrow producer-carried
signature metadata surface.

## Scope

- Inventory backend/BIR consumers that parse or inspect
  `LirFunction::signature_text` for semantic function signature facts.
- Review the existing structured LIR signature fields:
  - `return_type`
  - `params`
  - `signature_return_type_ref`
  - `signature_param_type_refs`
  - declaration/definition and variadic metadata
- Add or thread any missing structured metadata needed by BIR/backend lowering
  for return lowering, parameter lowering, aggregate/byval/sret decisions, and
  ABI-relevant signature classification.
- Convert BIR/backend function-signature lowering to prefer structured metadata
  and stop parsing `signature_text` as semantic authority for covered facts.
- Convert LIR verifier function-signature checks so structured metadata is the
  primary source of signature facts. Any `signature_text` parsing retained by
  the verifier must be limited to final-render consistency checks or fallback
  diagnostics.
- Keep `signature_text` available to the LIR printer, final-render consistency
  checks, diagnostics, dumps, and final emitted LLVM spelling.
- Add focused tests proving backend/BIR lowering survives drifted or corrupted
  `signature_text` when structured metadata remains correct.

## Out Of Scope

- Removing `LirFunction::signature_text`.
- Changing final LLVM header rendering.
- Parser or HIR source lookup cleanup.
- Broad type-system redesign beyond signature metadata needed at the LIR ->
  BIR/backend boundary.
- MIR or target assembly behavior changes except where they directly consume
  the newly structured signature facts.
- Weakening supported tests or replacing semantic checks with string-shaped
  shortcuts.

## Acceptance Criteria

- Backend/BIR lowering no longer parses `signature_text` for covered function
  return/parameter ABI facts.
- Structured LIR signature metadata carries the facts needed by BIR/backend
  function-signature lowering.
- LIR verifier function-signature checks use structured metadata as the primary
  fact source; `signature_text` parsing, if retained, is limited to validating
  final rendered spelling or producing fallback diagnostics.
- `signature_text` use is limited to final rendering, diagnostics, dumps,
  final-render consistency checks, or explicitly classified fallback paths.
- Tests demonstrate that structured signature metadata, not rendered
  `signature_text`, is the semantic authority when the rendered spelling drifts.
- Any remaining `signature_text` consumers that still act as semantic authority
  are listed in a follow-up idea instead of being left implicit.

## Closure Note

Completed by threading structured LIR signature metadata for return,
parameter, variadic, void-parameter-list, aggregate, and ABI-relevant facts,
then converting backend/BIR lowering, LIR verifier semantic checks, and
aarch64 fast-path signature gates to use that metadata as the primary
authority.

Remaining `signature_text` uses are limited to final header rendering,
diagnostics, renderability/final-spelling consistency checks, producer
compatibility scanning, or documented legacy fallback paths for hand-built LIR
without structured metadata. No remaining suspicious semantic authority was
found for generated LIR, so no follow-up idea is required for this boundary.

Close-time regression guard compared the full-suite baseline in
`test_baseline.log` against `test_after.log`: 3088 passed before, 3089 passed
after, 0 failed, and 0 new failures.

## Superseded Earlier Route

An earlier version of this idea targeted `collect_fn_refs` scanning
`signature_text` for signature-embedded function references. Review did not find
a concrete function-reference family that required a new producer-carried
reference surface. The real cross-IR string legacy in this area is backend/BIR
function-signature metadata recovery from final rendered header spelling.
