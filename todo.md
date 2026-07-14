# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.32
Current Step Title: Verify direct external double call-result authority

## Just Finished

- Completed Plan Step 7.31: a direct, fixed, nonvariadic, zero-argument
  native `long double` call on AArch64 now allocates a fresh `LirValueId` for
  its actual `fp128` LIR return and preserves that exact ID into a same-type
  ordinary FAdd.
- The native-only floating verifier admits `fp128` alongside float, double,
  and x86_fp80 without source-spelling recovery. Focused AArch64 coverage
  rejects missing, invalid, duplicate, unknown, cross-owner, callee,
  signature, return, opcode, and binary-type authority conflicts while
  display-only mutations remain compatible.

## Suggested Next

- Bounded packet 7.32: publish only the existing native-authority seam for a
  direct, fixed, nonvariadic, zero-argument external `double` call. Its
  already-fresh `LirCallOp.result` must remain the exact ID used by one
  same-type FAdd; reachable verification must recognize the call's
  `direct_callee_link_name_id` through the matching link-ID-keyed external
  declaration rather than requiring a local function body.

## Watchouts

- Own only the external declaration's native `LinkNameId`, exact `double`
  return `LirTypeRef`, `None` return extension, and fixed empty signature;
  require its declaration facts to agree with the call, while preserving the
  exact result-to-FAdd ID/type edge. The external name, callee/result/use
  displays, raw signature mirrors, and declaration order remain presentation
  only and must never create, repair, or select authority.
- Exclude call arguments, indirect or variadic calls, ABI-expanded calls,
  other external return types, conversions, other floating operations, BIR,
  and every separate family. Do not broaden the accepted internal
  float/double/x86_fp80/fp128 rows or introduce a cross-operation provenance
  carrier.

## Proof

- Fresh `cmake --build --preset default` plus `ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure`, covering one external
  double-call-to-FAdd chain; display-only positive mutations; and rejection of
  missing/invalid/duplicate/unknown/cross-owner result IDs, unresolved or
  mismatched external declaration IDs, return type/extension/signature
  conflicts, and wrong FAdd opcode/type/use authority. Run `git diff --check`
  before handoff.
