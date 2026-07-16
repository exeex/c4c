# LIR Family-Overloaded Verifier, Dispatch, and Printer

Status: Closed
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

## Closure Disposition

Closed after supervisor-accepted bounded verifier/printer consumer migrations
made selected native-authoritative LIR type-family consumers use nominal family
facts while treating stale display mirrors as check-only rendering parity.

Accepted route evidence includes:

- `285f16c71`: integer `va_arg` type rendering
- `89a92deec`: insert-element vector type rendering
- `857059a61`: extract-element vector type rendering
- `7b20c190c`: shuffle-vector type rendering
- `134707b5f`: extractvalue aggregate type rendering
- `5107e300e`: insertvalue aggregate type rendering
- earlier accepted scalar PHI boundary, scalar cast endpoint, integer GEP
  index, integer alloca, and related verifier/printer boundary packets in this
  route

The latest accepted proof for the route included a fresh build,
`ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
recorded in `test_after.log`, `git diff --check`, and a canonical focused
before/after guard accepted with `--allow-non-decreasing-passed`.

The remaining obvious string-backed `LirSwitch` selector printer/verifier
surfaces are intentionally outside this idea's non-goals and remain owned by
the separate switch routes `821` and `822`. Remaining generic
`require_type_ref` and `require_module_type_ref` callers are legacy,
compatibility, inline-assembly, call/signature compatibility, or other
unselected surfaces without a selected native-authoritative fact published by
this 846 route.

Next lifecycle action: activate
`ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md` when the
supervisor is ready to perform the terminal deletion pass for generic helpers,
mutable semantic string escape hatches, implicit string conversions, and
expired compatibility adapters. This closure does not claim that deletion work
is complete.
