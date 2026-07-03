# BIR Direct-Call Semantic Boundary Evidence

Status: Closed
Type: Focused BIR semantic producer evidence gap
Parent: `ideas/closed/547_bir_local_memory_call_metadata_boundary_review.md`
Owning Layer: BIR semantic call lowering

## Goal

Name the missing semantic direct-call fact behind retained
`semantic_lir_to_bir_direct_call_family` rows, then repair only that BIR-owned
boundary if the evidence proves a producer gap.

## Why This Exists

The boundary review retained 26 direct-call semantic-family rows from per-case
RV64 torture logs. Representative cases such as `src/20000419-1.c`,
`src/20000717-1.c`, `src/20040703-1.c`, and `src/pr67226.c` fail before the
prepared object handoff with `semantic call family 'direct-call semantic
family'`.

Step 3 placed the first boundary in `BirFunctionLowerer::lower_call_inst`,
which owns direct-call callee classification, signature/return ABI, argument
lowering, byval/sret handling, and `CallArgumentSourceRelationship`
publication. The retained logs do not yet name which direct-call fact is
absent, so a focused evidence pass is required before implementation.

## In Scope

- Rerun one retained representative, preferably `src/20000717-1.c` or
  `src/pr67226.c`, through semantic BIR and, if reached, prepared BIR.
- Inspect `lower_call_inst` failure notes, `CallInst` argument and return ABI
  rows, `call_arg_source` annotations, route6 call argument
  source/producer/publication records, call-result records, and any byval/sret
  aggregate layout facts.
- Preserve the BIR-owned classification only for a named missing semantic
  fact.
- Add focused BIR route6/printer coverage before relying on RV64 proof.
- If a repair is made, keep it limited to the named direct-call semantic
  producer fact.

## Out Of Scope

- RV64 call lowering changes that guess missing argument, return, byval, or
  sret facts.
- Treating no-count call-metadata suspicions as implementation-ready.
- Combining direct-call semantic repair with local-memory
  base-plus-offset work.
- Broad BIR call-route rewrites unrelated to the named missing direct-call
  fact.

## Acceptance Criteria

- A focused proof names the representative case and the exact absent or
  incoherent direct-call fact.
- The evidence distinguishes callee classification, argument source
  publication, return result publication, byval/sret handling, and prepared
  handoff failures.
- Any implementation adds or extends focused BIR call-publication coverage
  before representative RV64 proof.
- If no named missing semantic fact is found, the rows remain an evidence gap
  rather than being converted into a repair route.

## Completion Notes

Closed after Step 5 of the active runbook. The route named the BIR-owned first
bad fact as byval aggregate direct-call publication, added focused BIR coverage
for that fact, repaired the BIR producer boundary, and proved
`tests/c/external/gcc_torture/src/20000717-1.c` through semantic BIR, prepared
BIR, and RV64 object generation.

Close-gate validation used the backend CTest subset with matching
`test_before.log` and `test_after.log` captures. The representative no longer
has a direct-call semantic boundary failure in the covered semantic,
prepared, or RV64 object routes. Remaining select-carrier publication limits
observed in the prepared dump are outside this direct-call semantic-boundary
idea and did not block the proof.

## Reviewer Reject Signals

- Reject RV64 call lowering that recovers argument, return, byval, or sret
  metadata from target-specific call shapes instead of consuming BIR/prepared
  facts.
- Reject claiming BIR call capability progress without a focused dump naming
  the missing direct-call semantic fact.
- Reject testcase-shaped shortcuts for `src/20000419-1.c`,
  `src/20000717-1.c`, `src/20040703-1.c`, `src/pr67226.c`, or any other named
  torture case.
- Reject expectation rewrites, unsupported downgrades, diagnostic renames, or
  helper-only refactors as direct-call semantic repair.
- Reject broad `lower_call_inst` rewrites that leave the old
  `semantic_lir_to_bir_direct_call_family` failure mode behind a renamed
  abstraction.
- Reject repairs that skip focused BIR call-publication coverage and prove
  only a narrow RV64 testcase.
