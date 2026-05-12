# LIR Call Argument Structured Payload Boundary

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/184_direct_call_signature_metadata_structured_boundary.md`
- `ideas/closed/189_direct_call_no_prototype_variadic_signature_mismatch.md`

## Goal

Give metadata-rich LIR call sites a structured argument payload boundary so
LIR-to-BIR lowering does not need to recover ordinary call argument type and
operand information from `callee_type_suffix` / `args_str` text.

Rendered call text should remain output/compatibility spelling. Structured
metadata should be the authority for generated call argument lists whenever it
is available.

## Why This Idea Exists

Direct-call signature metadata now exists, but BIR lowering still routes much
of the actual call argument view through formatted LIR call text parsers. That
keeps the backend close to the old model where final spelling is parsed back
into semantic facts.

Before backend restart, the generated metadata-rich path should carry enough
structured call argument information that text parsing is only a raw/no-id or
printer/verifier compatibility path.

## In Scope

- Inventory `LirCallOp`, `LirCallSignature`, `call_args.hpp`, and LIR-to-BIR
  call parsing helpers.
- Add or select a structured call argument carrier for generated metadata-rich
  calls: argument operand, argument type ref/text mirror, byval/sret/variadic
  markers as needed by BIR.
- Use the structured carrier in LIR-to-BIR call lowering when present.
- Keep `callee_type_suffix` and `args_str` as final spelling and raw
  compatibility payload.
- Add focused tests where stale rendered call text cannot override structured
  argument metadata.

## Out Of Scope

- Rewriting all LIR instruction operands in one slice.
- Removing printed call syntax or raw hand-authored LIR fixtures.
- Redesigning target ABI lowering.
- Reopening frontend overload resolution unless the structured payload exposes
  a concrete missing fact.

## Acceptance Criteria

- Metadata-rich generated call lowering can obtain argument facts without
  reparsing `callee_type_suffix` / `args_str` as semantic authority.
- Stale rendered call text cannot silently change selected argument type or
  ABI facts when structured metadata is present.
- Raw/no-metadata compatibility remains explicit.
- Validation includes focused LIR-to-BIR direct and indirect call coverage.

## Completion Notes

- `LirCallOp` now carries structured argument facts populated from generated
  `OwnedLirTypedCallArg` values.
- LIR-to-BIR call lowering uses non-empty structured argument payloads as the
  metadata-rich authority for operand, type text, and byval type-ref facts.
- Empty structured payloads remain the explicit raw/no-metadata compatibility
  path through legacy rendered-text parsing.
- Focused coverage proves stale rendered call text does not override structured
  metadata for direct scalar, direct byval, indirect, and no-signature
  structured calls.
- Close-time full-suite regression guard passed with `3137/3137` runnable tests
  green before and after.

## Reviewer Reject Signals

- The implementation only renames text parsers without adding a structured
  call argument authority.
- Generated metadata-rich calls still prefer parsed rendered call text.
- Tests only assert printer output.
- The slice turns into a broad backend ABI rewrite.
