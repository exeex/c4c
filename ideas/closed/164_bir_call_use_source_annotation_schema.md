# 164 BIR call-use source annotation schema

## Goal

Prototype BIR call instruction and value annotations for call-boundary semantic
source facts established by Phase A Route 6.

## Why This Exists

Phase A proved BIR-owned call argument/result source identity for the
source-producer and direct-global dependency families. Phase B classified these
as `CallInst` instruction annotations with value annotations and function
indexes only for lookup, while preserving ABI/layout-bound reads as prepared or
codegen-owned.

## In Scope

- Define `CallInst` annotations for argument and result source value/base
  value/name, same-block producer links, direct-global select-chain dependency,
  and semantic memory/publication source references where they do not require
  ABI or layout authority.
- Define value annotations for call result provenance where consumers start
  from the produced result value.
- Keep prepared call argument/result source materialization and direct-global
  dependency reads as migration oracles.
- Split semantic call-use facts from ABI-bound publication-routing reads.

## Out Of Scope

- ABI register or stack placement, outgoing stack sizing, variadic FPR count,
  preservation/clobber sets, byval aggregate lanes, scratch requirements,
  destination homes, helper/carrier protocols, aggregate transport lane layout,
  and ABI/layout-bound publication-routing source-selection reads.

## Acceptance Criteria

- BIR-owned call annotations answer semantic source facts without exposing ABI,
  layout, scratch, or transport policy.
- ABI-bound reads remain prepared/codegen-owned until separately justified.
- Existing prepared call source queries can prove BIR answer equivalence during
  migration.

## Reviewer Reject Signals

- Reject if annotations include ABI register names, stack slots, outgoing stack
  area sizing, variadic FPR counts, clobber/preservation sets, byval lane
  layout, scratch resources, helper/carrier protocol, or aggregate transport
  layout.
- Reject if `PreparedCallArgumentPlan`, call result plans, outgoing-stack
  plans, or aggregate transport lane records are copied into BIR.
- Reject if semantic call source facts are proven only for one named argument
  case while result, direct-global, publication-source, and ABI-bound negative
  cases remain unexamined.
- Reject expectation rewrites that weaken call-boundary source contracts rather
  than preserving old prepared answers as oracles.
- Reject broad rewrites of call lowering or ABI assignment as schema progress.

## Closure Notes

Closed after implementing Route 6 BIR call-use source records and indexes over
`CallInst` and value payloads, oracle tests for semantic call argument/result
source facts, direct-global dependency, eligible memory/publication source
facts, and ABI-bound exclusions, plus a low-risk migration of
`bir::find_call_argument_source_producer_materialization(...)` to Route 6
records.

Reviewer report `review/164_call_use_source_route_review.md` found no
blocking issues and judged closure reasonable. The migration shape is accepted:
there was no dedicated public MIR call-use query in this route, and no broad
MIR, target/codegen, ABI assignment, aggregate transport, or prealloc
production consumer was switched.

Closure validation used canonical broad backend logs from:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

`test_before.log` and `test_after.log` both passed `179/179`, and the
regression guard passed with no new failures or pass-count regression.
