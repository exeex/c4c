# RV64 Selected Object-Data Emission

Status: Closed
Source Parent: ideas/open/424_prepared_global_stack_frame_infrastructure_review.md
Handoff: docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
Owning Layer: RV64 object/global-data emission

## Completion Notes

Closed after completing the RV64 consumer slice for prepared-authoritative
selected object-data records.

The implemented route admits only prepared selected object-data records with
explicit object label, extent, initialized-byte, zero-fill, section, alignment,
and relocation authority. It emits initialized data, reserves zero-fill, defines
object symbols, and records relocations from those prepared facts rather than
reconstructing storage from source shape, raw globals, or testcase identity.

Focused object-emission and ordinary-C backend coverage prove initialized data,
zero-fill, relocation, and fail-closed missing-authority behavior. Backend
regression guard passed for the canonical `^backend_` before/after logs:
`test_before.log` had 331 passing tests and `test_after.log` had 339 passing
tests, with no new failures.

Remaining rows such as `tests/c/external/gcc_torture/src/20000412-1.c` and
`tests/c/external/gcc_torture/src/930513-2.c` remain fail-closed because the
prepared producer does not yet publish enough authority:

- `20000412-1.c` is still marker-only `unsupported_but_coherent` with no
  prepared initialized bytes, zero-fill, or relocation authority.
- `930513-2.c` still reports `missing_object_label`.

Those producer gaps are follow-up work, not incomplete RV64 consumer work under
this idea. Follow-up initiatives should cover static-local object-label
publication, byte/zero-fill fact publication, relocation-ready object-data
records, and any broader TLS/GOT/F128/global-width/ABI scope separately.

## Goal

Teach RV64 object emission to emit coherent selected object-data contracts,
including object extents, initialized data, zero-fill, and relocations that
are already published by prepared facts.

## Why This Exists

Idea 424 classified `src/20000412-1.c` as a coherent RV64 emission gap.
Prepared addressing for `i` is coherent, and selected object data reports an
object label and object size, but the object route still rejects the selected
object-data contract:

- `prepared selected object-data contract status=unsupported_but_coherent object_label_id=2`

Representative evidence:

- Row: `src/20000412-1.c`
- Bucket: `unsupported_global_data`
- Artifacts:
  `build/agent_state/424_step2_infrastructure_classification/20000412-1/`
- Handoff docs:
  `docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md`
  and
  `docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md`

Known prepared facts from the handoff:

- Direct prepared addressing for `i` is coherent.
- Selected object data reports `status=unsupported_but_coherent`.
- `object_label_id=2`
- `object_size_bytes=1656`
- `emitted_byte_count=0`
- `zero_fill_byte_count=0`

## In Scope

- RV64 object/global-data emission for prepared selected object-data contracts
  that are coherent and have explicit object label and extent authority.
- Emission of object data, zero-fill ranges, and relocations from prepared
  contract records.
- Focused ordinary-C backend coverage for a selected object-data case.
- Fail-closed diagnostics for missing labels, missing extents, unsupported
  widths, ambiguous relocations, or incoherent object-data contracts.

## Out Of Scope

- Producer repair for missing static-local object labels or object extents.
- Reconstructing arrays, string data, or zero-fill from C source shape,
  symbol spelling, or testcase-specific globals.
- General unsupported global-memory access widths or unrelated global-address
  modes not covered by coherent selected object-data contracts.
- Expectation, unsupported-marker, allowlist, runtime-comparison, or scan
  accounting changes.

## Acceptance Criteria

- RV64 emits coherent selected object data from prepared contract records.
- `src/20000412-1.c` or a focused ordinary-C equivalent advances through the
  selected object-data route being repaired.
- Tests prove initialized-data and/or zero-fill behavior through object-route
  execution or object inspection, not through scan-bucket accounting.
- Missing-object-label rows such as `src/930513-2.c` remain fail-closed until
  producer facts exist.
- Backend proof includes the focused coverage and an appropriate `^backend_`
  subset.

## Reviewer Reject Signals

Reject any route or slice that:

- special-cases `src/20000412-1.c`, `wordlist`, object label `2`, object size
  `1656`, or one exact initializer layout
- infers object labels, data bytes, zero-fill, or relocations in RV64 from
  source names or raw global symbols instead of prepared selected object-data
  records
- combines this RV64 consumer work with producer repair for missing object
  labels or extents
- changes expectations, unsupported markers, allowlists, runtime comparison,
  or pass/fail accounting instead of adding object-data emission
- claims progress through classification-only edits or diagnostic renames
- allows missing-object-label producer gaps to pass by reconstructing storage
  in the target backend
