# Destination Fan-In Authority Research Index

## Research Question

This package answers whether non-parallel move bundles may choose among
multiple register sources targeting one stack destination, and what authority
must exist before RV64 target materialization may consume that shape.

The researched population is the July 8 RV64 gcc_torture recovery-map family
of `125` rows classified as `non-parallel multi-source stack destination`.
The representative current diagnostics show
`ambiguous_non_parallel_multi_source_stack_destination`,
`unsupported_prepared_move_bundle_classification: non-parallel`,
`authority=none`, `parallel_copy=no`, and register-source fan-in to a shared
stack-slot destination.

## Answer Files

- [01_current_failure_shapes.md](01_current_failure_shapes.md) defines the
  family boundary, representative rows, diagnostic vocabulary, and adjacent
  owner exclusions.
- [02_destination_authority_rule.md](02_destination_authority_rule.md)
  evaluates ordering, mutual exclusion, merge authority, and explicit
  rejection, then selects explicit rejection until producer destination
  authority exists.
- [03_implementation_split.md](03_implementation_split.md) separates
  prepared/prealloc producer authority work from downstream RV64 consumption
  and lists rows that must remain rejected or blocked.

## Evidence Base

The package uses:

- the current scan summary and failure bucket map under
  `docs/rv64_gcc_torture_1000_pass_recovery/`;
- representative backend logs such as
  `build/rv64_gcc_c_torture_backend/src_pr43236.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000113-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20040409-1w.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20021010-2.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20021120-3.c/case.log`, and
  `build/rv64_gcc_c_torture_backend/src_pr48814-2.c/case.log`;
- source-freshness context from
  `ideas/closed/587_prepared_value_freshness_authority_mvp.md` and
  `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`;
- prepared/prealloc classifier surfaces in
  `src/backend/prealloc/prepared_object_traversal.hpp` and
  `src/backend/prealloc/prepared_object_traversal.cpp`.

Source freshness authority is necessary context, but it is not destination
authority. A fresh source does not prove that two non-parallel register
sources may write the same stack destination.

## Selected Rule

Selected rule: explicit rejection.

A non-parallel move bundle with more than one register source targeting the
same stack destination must fail closed unless the prepared/prealloc producer
publishes explicit destination authority at the consumer program point.

Acceptable future producer authority must be one of:

- ordering authority with a designated final stack-destination state;
- mutual-exclusion authority with predicates, edges, or carrier metadata
  proving exactly one active candidate; or
- merge authority with semantic equivalence or an explicit merge operation.

The research explicitly rejects destination choice by testcase shape, source
order accident, shared-destination inference, source-freshness substitution,
or RV64 target convenience.

## Implementation Split

Producer/prealloc owns destination authority production. Any follow-up
producer implementation must publish one concrete authority contract and prove
negative cases still fail closed when authority is missing, stale, incomplete,
unsupported, or mismatched.

RV64 owns only downstream consumption after producer authority exists. RV64
may materialize ordered, mutually exclusive, or merge-authorized bundles only
when the producer has already published the required authority and source
freshness remains valid. RV64 must preserve rejection for `authority=none`,
unsupported authority kinds, mismatched bundle-versus-move authority, and
missing source freshness.

The current `125` row family remains blocked from implementation acceptance.
The explicit-rejection rule is implementation-ready as a fail-closed rule, not
as permission to materialize the current rows.

## Acceptance Check

The source idea acceptance criteria are satisfied for the research package:

- `docs/destination_fan_in_authority/` contains exactly this `index.md` plus
  the three numbered answer files;
- the documents cite concrete current logs, diagnostics, and prepared/prealloc
  code or documentation surfaces;
- the conclusion defines the implementation-ready rule as explicit rejection
  until producer destination authority exists;
- the route remains documentation-only, with no implementation, expectation,
  unsupported-marker, allowlist, runtime, timeout, or accounting changes.

Closure recommendation: the supervisor can route this research idea to the
plan owner for closure. Implementation should remain blocked until a separate
producer-authority idea is opened for exactly one destination authority
contract; RV64 consumption should wait for that producer fact.
