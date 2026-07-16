# LIR Family-Overloaded Verifier, Dispatch, and Printer

Status: Open
Type: first-owner family consumer migration
Matrix Rows: M12, M13
Dependencies: 838, 839, 840, 841, 842, 843, 844

## Goal

Replace universal classification with nominal verifier, semantic dispatch, and
one-way printer overloads by family.

## In Scope

- Add `require_scalar`, `require_vector`, `require_aggregate`,
  `require_signature`, and value overloads; aggregate checks module/store and
  recursive children.
- Migrate named callers and matching one-way render/dispatch overloads.

## Out Of Scope

- Producer/store construction, universal-model deletion before all callers
  migrate, or parsing display text as semantic state.

## Acceptance Criteria

- Fresh build plus valid/malformed/foreign/wrong-module/wrong-family verifier,
  exhaustive dispatch, and contractual byte-for-byte rendering parity.
- Delete generic helpers/renderers, mutable semantic `.str()`, and
  `LirTypeRef(type.str())` classification only after every named caller uses an
  overload and mirrors are check-only, never parsed.

## 866 Reconciliation And 734 Return

Idea 866 orders this after family producers and carriers exist. It may enable a
later 734 receiver only by accepting verifier/dispatch proof for one already
published native fact; it must not create producer state or treat printer
parity as semantic authority.

## Reviewer Reject Signals

- Reject generic kind switches or text reclassification behind overload names,
  weakened rejection, printer-driven semantics, or partial-callsite deletion.
