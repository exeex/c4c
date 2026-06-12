# 218 Phase E1 semantic duplicate route fact migration

## Goal

Move only the prepared surfaces that Phase E0 classifies as semantic duplicates
into BIR-owned route facts or BIR annotations, one surface at a time.

This phase is implementation-oriented only after E0 names accepted candidates.
Until E0 closes, this draft is a placeholder for the expected next class of
work and must not be opened.

## Prerequisite

Do not open this draft until
`docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md` exists and
names one or more E1 candidates.

## Direction

Each follow-up must target exactly one semantic duplicate surface. Valid work
should remove duplicate semantic authority from prepared while preserving
prepared fallback/oracle behavior during the transition.

## Candidate Shape

Examples may include, only if E0 accepts them:

- memory/source identity facts that are semantic rather than target-addressing
  policy;
- publication/source identity facts that are semantic rather than publication
  mechanics;
- call-use source identity facts that are semantic rather than ABI/call-record
  policy;
- comparison provenance facts that are semantic rather than branch spelling or
  instruction selection policy.

## Out Of Scope

- Target policy migration.
- Public fallback removal before equivalent route-native proof exists.
- Diagnostic/oracle contraction.
- Aggregate `PreparedFunctionLookups` or `PreparedBirModule` retirement.
- Broad route-family migration from one selected reader.

## Expected Output

The closure note for any activated E1 child idea must name:

- the exact semantic duplicate surface;
- the BIR route fact or annotation that becomes authoritative;
- the prepared helper retained as fallback or oracle;
- positive, absent, invalid, duplicate/conflict, mismatch, fallback, and
  expected-string proof;
- whether a later E2 public API contraction candidate was created.

## Reviewer Reject Signals

- Moving ABI/layout/register/stack/emission policy into BIR.
- Replacing one prepared helper with a route-named wrapper while retaining the
  same prepared-only authority.
- Weakening expectations, supported-path status, helper names, diagnostics, or
  baseline proof.
