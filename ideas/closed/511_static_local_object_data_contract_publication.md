# Static-Local Object-Data Contract Publication

Status: Closed
Source Parent: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Handoff: docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
Owning Layer: producer object-data contract

## Completion Notes

Closed after Step 6 validation completed the producer-side static-local
object-data publication scope.

Function-scope static storage now publishes stable prepared object-data
authority through the producer contract: object identity, size, alignment,
initialized data, and zero-fill facts are available for RV64 consumers without
target-side reconstruction from access facts, symbol spelling, source shape, or
the representative torture row.

Focused static-local/prepared object-data coverage passed for initialized and
zero/default function-scope static objects. The representative
`tests/c/external/gcc_torture/src/930513-2.c` now passes asm and object codegen,
with asm showing `__static_local_eq_0:`, `.zero 4`, and loads/stores through
that prepared label.

Backend regression guard passed for the canonical `^backend_` before/after
logs: `test_before.log` had 331 passing tests and `test_after.log` had 344
passing tests, with no new failures.

Remaining non-goals are follow-up scope, not incomplete work for this idea:
static-local symbol visibility/local binding if required later, GOT-required
globals, thread-local storage, dynamic initialization, stack-passed parameter
homes, broad global access widths, and unrelated RV64 consumer gaps.

## Goal

Publish prepared object-data authority for function-scope static storage,
including stable object labels, object size, initialized data, zero-fill
extents, alignment, and relocation-ready identity.

## Why This Exists

Idea 424 classified `src/930513-2.c` as a producer-contract gap. Prepared BIR
has direct global accesses for `__static_local_eq_0`, but selected object data
has no storage-object authority:

- `status=missing_object_label object_size_bytes=0 emitted_byte_count=0 zero_fill_byte_count=0`

Representative evidence:

- Row: `src/930513-2.c`
- Bucket: `unsupported_global_data`
- Artifacts:
  `build/agent_state/424_step2_infrastructure_classification/930513-2/`
- Handoff docs:
  `docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`
  and
  `docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md`

Known missing authority from the handoff:

- Static-local object label for `__static_local_eq_0`.
- Object size and alignment.
- Data and/or zero-fill extents.
- Storage-object identity that RV64 can consume without reconstructing it.

## In Scope

- Producer-side publication of selected object-data contract records for
  function-scope static storage.
- Object labels and object extents for static locals that already have direct
  global access facts.
- Data and zero-fill extent records precise enough for target emission.
- Focused contract/printer/backend evidence showing RV64 can see the object
  facts and still fails closed only on consumer support if needed.

## Out Of Scope

- RV64 emission of selected object data once producer facts are coherent.
- Inferring static-local storage in RV64 from symbol spelling, access facts, or
  source shape.
- Broad global-data cleanup for unrelated unsupported access widths or
  addressing modes.
- Expectation, unsupported-marker, allowlist, runtime-comparison, or scan
  accounting changes.

## Acceptance Criteria

- Prepared output publishes a stable object label, size, alignment, and
  data/zero-fill extents for the representative static-local object.
- Access facts and object-data facts agree on the object identity RV64 should
  consume.
- `src/930513-2.c` or a focused ordinary-C equivalent moves from
  `missing_object_label` to a coherent selected object-data status.
- RV64 does not reconstruct missing facts; if RV64 consumer support is still
  incomplete, the remaining diagnostic identifies the consumer gap.
- Proof includes focused producer/prepared dumps and the relevant backend
  subset.

## Reviewer Reject Signals

Reject any route or slice that:

- special-cases `src/930513-2.c` or `__static_local_eq_0`
- makes RV64 synthesize object labels, extents, bytes, or zero-fill from access
  facts, symbol names, or source shape
- combines producer publication with RV64 selected object-data emission in the
  same implementation idea
- changes expectations, unsupported markers, allowlists, runtime comparison,
  or pass/fail accounting instead of publishing prepared authority
- claims capability progress from printer-only changes, diagnostics reshaping,
  or classification edits
- leaves `status=missing_object_label` behavior intact behind a new contract
  name
