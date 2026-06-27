# Prepared Packed And FP128 Global Initializer Admission

Status: Open
Type: Producer-side follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/399_rv64_object_route_global_data_and_symbol_memory.md`

## Goal

Repair BIR/prepared producer admission for packed aggregate and `fp128`
global initializers so supportable global storage facts reach RV64 object
emission explicitly.

## Why This Exists

The 2026-06-27 Step 1 refresh for idea 399 found that the RV64 object-route
global-data diagnostics are stale or passing for the source representatives:
`20010924-1`, `20001121-1`, `20031211-1`, and `pr57568` all compile and dump
prepared BIR successfully.

The remaining `tests/c/external/gcc_torture/src/20040709-2.c` evidence stops
before prepared handoff in producer/global admission. Its visible shape is a
large set of packed aggregate globals such as `%struct.A <{ i16 0 }>` through
`%struct.Z`, including `fp128`-containing globals. That is not a target
object-emitter packet. The producer must either publish explicit prepared
global initializer/storage facts for supportable packed and `fp128` global
forms, or reject them with a precise producer-side diagnostic.

## In Scope

- Classify the packed aggregate and `fp128` global initializer shapes in
  `20040709-2.c`.
- Identify which producer/global admission layer currently rejects them before
  prepared handoff.
- Publish explicit prepared global storage facts for supportable packed
  aggregate and `fp128` initializer forms when byte layout, alignment, and
  object representation are unambiguous.
- Preserve precise unsupported diagnostics for unsupported or ambiguous
  packed, floating, or aggregate global initializer forms.
- Prove `20040709-2.c` and focused backend/prepared global initializer
  coverage.

## Out Of Scope

- Reopening RV64 object-emitter global-data lowering from idea 399 unless
  fresh prepared facts reach that consumer and expose a distinct target bug.
- Teaching RV64 object emission to reconstruct packed aggregate layout,
  `fp128` bytes, or source-level global initializer semantics.
- Reopening unrelated direct-call semantic, memset runtime, or vector local
  alloca admission failures from the same stale-log refresh.
- Broad frontend constant-evaluator rewrites beyond the packed/`fp128` global
  initializer admission boundary.
- Expectation rewrites, unsupported downgrades, allowlist filtering, or
  testcase-specific shortcuts.

## Acceptance

- `src/20040709-2.c` no longer stops first because supportable packed
  aggregate or `fp128` global initializers lack prepared admission.
- Prepared output contains explicit global storage facts for the supported
  packed/`fp128` initializer family, or a precise producer-side unsupported
  diagnostic for forms still out of scope.
- Any later RV64 object-route, link, qemu, or runtime boundary is routed to an
  existing or new owner instead of being absorbed into this producer idea.
- Backend/prepared proof shows no regression in existing global storage,
  initializer, and RV64 object-emission coverage.

## Reviewer Reject Signals

- Reject filename-specific handling for `20040709-2.c` or matching the exact
  `%struct.A` through `%struct.Z` names.
- Reject RV64 object-emitter code that synthesizes packed aggregate layout or
  `fp128` initializer bytes instead of consuming producer-published facts.
- Reject merely renaming the admission failure while supportable packed or
  `fp128` global initializers still cannot reach prepared output.
- Reject broad global initializer rewrites that do not directly prove the
  packed/`fp128` producer admission boundary.
- Reject expectation downgrades, unsupported markers, allowlist filtering, or
  diagnostic-only churn claimed as capability progress.
