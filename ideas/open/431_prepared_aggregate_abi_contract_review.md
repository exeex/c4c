# Prepared Aggregate ABI Contract Review

Status: Open
Type: Producer contract review idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Source Evidence: `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
Owning Layer: Prepared ABI producer first, then RV64 aggregate call lowering
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Separate coherent aggregate `sret`/`byval` call-storage rows from prepared ABI
producer defects before any RV64 aggregate lowering work begins.

## Why This Exists

The regenerated RV64 gcc_torture scan classified 19
`unsupported_instruction_fragment` rows as aggregate `sret`/`byval`
call-storage. Representative rows include `src/pr88904.c`,
`src/20000917-1.c`, `src/20010224-1.c`, `src/20020206-1.c`, and
`src/20020506-1.c`; Step 2 noted suspicious prepared aggregate sizes or
alignments in some rows, so this cannot be treated as ordinary scalar RV64
call publication.

## In Scope

- Inspect prepared aggregate ABI facts for `sret` and `byval` rows, including
  size, alignment, storage class, and call-home ownership.
- Classify rows as coherent RV64 aggregate call-storage work or prepared
  producer defects.
- Add fail-closed contract tests for suspicious aggregate size/alignment facts
  if producer defects are confirmed.
- Produce a separate RV64 aggregate lowering idea only for rows with coherent
  prepared ABI facts.

## Out Of Scope

- Implementing RV64 aggregate call-storage lowering in this review idea.
- Treating aggregate rows as scalar call publication.
- F128 helper calls, long-double ABI, or runtime-helper policy.
- Reconstructing aggregate size/alignment in RV64 from source or raw BIR.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Acceptance Criteria

- Aggregate `sret`/`byval` rows are split into coherent prepared ABI facts
  versus producer defects.
- Suspicious size/alignment cases have a producer-owned repair route or
  fail-closed test contract.
- Any later RV64 aggregate lowering idea cites only coherent prepared ABI rows.
- The proof includes focused ABI contract tests or a docs-only classification
  proof if this remains review-only.

## Reviewer Reject Signals

- Reject RV64 lowering that infers missing aggregate size, alignment, storage
  class, return slot, or byval copy facts.
- Reject merging aggregate `sret`/`byval` work with scalar call-adjacent
  materialization.
- Reject claiming aggregate capability progress through classification-only
  edits, helper renames, expectation rewrites, or unsupported downgrades.
- Reject a route that treats suspicious prepared aggregate facts as acceptable
  target-lowering inputs without producer validation.
- Reject broad ABI rewrites that also pull in F128 helper calls or unrelated
  calling-convention work.
