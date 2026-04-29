# LIR Structured Signature Reference Producer Boundary

Status: Open
Created: 2026-04-29

Parent Ideas:
- [125_lir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/125_lir_legacy_string_lookup_removal_convergence.md)

## Goal

Replace LIR function-signature reference discovery that still scans rendered
`signature_text` with a structured producer-carried reference surface.

## Why This Idea Exists

The LIR legacy string lookup convergence work classified `signature_text` as
final LLVM header spelling plus template-comment compatibility payload.
`collect_fn_refs` still scans that rendered text because function references
embedded in signatures do not yet have a structured producer carrier. That path
is intentionally retained as a compatibility fallback, not accepted semantic
authority.

This follow-up owns the producer-boundary work needed to make those signature
references explicit instead of rediscovered from rendered spelling.

## Scope

- Identify the HIR-to-LIR producer path that emits function signature text with
  embedded reference spellings.
- Add or thread a structured signature-reference carrier for the references
  currently recovered by `collect_fn_refs`.
- Make LIR reachability prefer the structured carrier over rendered
  `signature_text` scanning.
- Keep `signature_text` as final LLVM header spelling, diagnostics, dump text,
  or compatibility payload where needed.
- Update focused LIR or backend route tests to prove structured signature
  references survive drifted rendered spelling.

## Out Of Scope

- Broad type-reference redesign unrelated to signature reference reachability.
- Parser source lookup cleanup.
- MIR or target assembly behavior changes.
- Removing final emitted LLVM spelling for function headers.
- Weakening supported tests or replacing semantic checks with string-shaped
  shortcuts.

## Acceptance Criteria

- Function signature references needed by LIR reachability are available from a
  structured producer-carried surface.
- `collect_fn_refs` no longer treats `signature_text` scanning as primary
  semantic authority for covered signature references.
- Retained `signature_text` use is classified as final spelling, display,
  diagnostics, dump text, or compatibility payload.
- Focused tests prove structured signature-reference identity wins when
  rendered signature spelling drifts.
