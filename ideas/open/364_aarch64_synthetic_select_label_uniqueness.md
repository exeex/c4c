# AArch64 Synthetic Select Label Uniqueness

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the AArch64 backend residual where synthetic labels emitted for
select/materialized-label lowering can be reused inside one generated assembly
file, causing assembler failures before runtime.

## Why This Exists

The post-331 backend inventory classified `00143` as the leading current
non-timeout residual because it has a concrete compile-stage first bad fact.
Generated assembly for `build/c_testsuite_aarch64_backend/src/00143.c.s`
defines synthetic labels such as `.Lselect_mat_1_24_164_37_true` and the
matching `_end` label more than once, so assembly fails on duplicate symbol
definitions.

Existing open ideas are only adjacent. The parked block-label ordering idea
352 covers basic block label placement, epilogue reachability, and fallthrough
ordering; it does not own duplicate synthetic select-label allocation. The old
`00143` scalar-cast source-publication thread is likewise out of scope because
the current failure is not the structured register-source diagnostic.

## In Scope

- Localize where AArch64 select/materialized-label lowering allocates or
  formats synthetic labels for branch/select lowering.
- Determine why two emitted synthetic select label regions in one function or
  assembly file can receive the same true/end label names.
- Repair label allocation so synthetic labels are unique across all relevant
  select/materialized-label emission sites without depending on one testcase,
  block number, instruction index, or label string.
- Add focused backend coverage for multiple synthetic select/materialized
  label regions that would collide before the repair.
- Prove `c_testsuite_aarch64_backend_src_00143_c` advances past the duplicate
  label assembler failure or passes.

## Out Of Scope

- Reopening basic block label emission ordering, fallthrough epilogue
  placement, or block visitation from idea 352 unless fresh evidence shows the
  duplicate synthetic labels come from that owner.
- Reopening scalar-cast source-publication, local storage/writeback sizing, or
  the old `00143` structured register-source diagnostic.
- Runtime value correctness for `00143` after the duplicate-label assembler
  failure is gone.
- Fixing pointer comparisons, indexed aggregate writeback, scalar FP
  arithmetic, conditional operators, file I/O return publication, `sizeof`,
  aggregate initializer layout, enum bit-field layout, or timeout cases from
  the same inventory pass.
- Changing expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or test contracts.

## Acceptance Criteria

- The duplicate label fact is localized to a concrete AArch64 synthetic label
  allocation, naming, or emission boundary.
- Focused backend coverage proves two or more synthetic select/materialized
  label regions in the same output cannot receive the same label names.
- The repair is general for synthetic select/materialized-label generation and
  does not hard-code `00143` or the observed label suffixes.
- `c_testsuite_aarch64_backend_src_00143_c` no longer fails with duplicate
  `.Lselect_mat_*` label definitions.
- Any remaining `00143` failure is reclassified by its new first bad fact
  before this idea is closed.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, `.Lselect_mat_1_24_164_37_true`, one `_end` label,
  one block number, one instruction number, one function, one stack offset, or
  one emitted instruction neighborhood instead of repairing synthetic label
  uniqueness generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or assembler/test contracts;
- claims progress through label text rewrites that only make the observed
  suffix look different while duplicate synthetic labels can still be emitted;
- broadens into basic block ordering, epilogue placement, scalar-cast source
  publication, runtime value correctness, aggregate writeback, scalar FP, or
  other backend inventory buckets without fresh first-bad-fact evidence and a
  lifecycle split;
- proves only the external `00143` representative while leaving nearby focused
  synthetic select/materialized-label collision behavior untested.
