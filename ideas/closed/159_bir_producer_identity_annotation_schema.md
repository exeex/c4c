# 159 BIR producer identity annotation schema

## Goal

Prototype a target-neutral BIR annotation schema for producer/source identity
facts established by Phase A Route 1.

## Why This Exists

Phase A proved BIR-owned answers for same-block producer nodes, immediate
constants, value/name/type identity, producer instruction index, materialization
availability, and producer kind vocabulary. Phase B classified those facts as
value and instruction annotation candidates, with function indexes only as
lookup aids. The remaining blocker is that producer-kind authority must move to
a BIR-owned vocabulary before schema consumers depend on it.

## In Scope

- Define the BIR-owned producer-kind vocabulary needed by same-block producer,
  constant, and materialization queries.
- Prototype value and instruction annotations for producer instruction/index,
  source value/name/type identity, immediate constants, and materialization
  availability.
- Keep the old prepared producer and constant queries as migration oracles until
  BIR answers prove equivalence.
- Preserve cheap same-block lookup by adding only indexes that point at typed
  annotation records.

## Out Of Scope

- `PreparedValueHome`, physical registers, storage state, emitted-register
  availability, spill/reload behavior, operand views, frame-slot layout, and
  final instruction order.
- Whole `PreparedSameBlockScalarProducer` or
  `PreparedSameBlockValueMaterializationQuery` copies as BIR schema.
- Target-specific producer categories that expose AArch64 lowering policy.

## Acceptance Criteria

- BIR owns producer/source identity through typed value and instruction
  annotations rather than publication-owned enum authority.
- Existing prepared producer and integer-constant query behavior has direct
  oracle proof against the BIR-backed answers.
- Query cost remains compatible with current same-block lookup patterns without
  requiring broad scans.

## Reviewer Reject Signals

- Reject if the route imports `PreparedValueHome`, register names, storage
  encodings, frame slots, emitted-register state, spill/reload behavior, or
  final instruction records into the annotation payload.
- Reject if producer kinds are renamed in place but remain owned by publication
  or prealloc code instead of a target-neutral BIR vocabulary.
- Reject if only one named same-block testcase is matched while nearby constant,
  instruction producer, and materialization-availability cases still rely on
  prepared-only answers.
- Reject if expectation rewrites or helper renames are claimed as schema
  progress without a typed BIR annotation container and oracle comparison.
- Reject broad rewrites of unrelated storage, register allocation, or AArch64
  emission paths.

## Closure Note

Closed after implementing the Route 1 producer identity schema as typed BIR
producer vocabulary, value/instruction producer records, same-block
producer/constant/materialization query APIs, and a function-local
`Route1ProducerIndex` lookup aid. Prepared producer and integer-constant
queries remain available as migration oracles, with oracle-equivalence coverage
in `backend_prepared_lookup_helper`.

The permitted low-risk consumer switch moved the generic MIR same-block
producer and integer-constant identity layer onto the BIR Route 1 query
surface without changing AArch64 emission paths, storage/register policy,
publication hooks, or expectation contracts.

Validation: narrow producer proof passed for
`backend_prepared_lookup_helper` and `backend_x86_shared_producer_query`.
Closure validation used canonical `test_before.log` and `test_after.log` for
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`; regression guard passed with
`--allow-non-decreasing-passed` at 179/179 before and 179/179 after, with no
new failures.
