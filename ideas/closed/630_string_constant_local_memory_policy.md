# String-Constant Local-Memory Policy

Status: Closed
Type: Implementation
Parent: `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
Related:
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/closed/308_rv64_string_literals_and_extern_calls.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: RV64 string-constant local-memory policy / prepared data consumer
Queue Order: 30
Prerequisites: string constant data identity, label, extent, byte contents, and
local-memory use authority must be explicit before RV64 consumes the access
Proof Surface: rows with `string_constant` local-memory bases that still stop
at `unsupported_local_memory_access`

## Goal

Define and implement the RV64 policy for local-memory accesses whose selected
base is a string constant, consuming explicit prepared string/data authority
without treating string literals as ordinary frame-slot local memory.

## Why This Exists

Idea 614 closed the generic selected local-memory consumer route after proving
only the explicit frame-slot family. Step 4 residuals found a separate string
constant local-memory family represented by rows such as `src/20000722-1.c`,
`src/20010123-1.c`, `src/20011109-2.c`, `src/20021204-1.c`,
`src/20030920-1.c`, `src/920429-1.c`, `src/930429-1.c`, `src/pr34415.c`,
`src/pr35800.c`, and `src/ptr-arith-1.c`.

These rows need string/data policy and authority checks, not another
frame-slot local-memory consumer shortcut.

## In Scope

- Refresh diagnostics for string-constant local-memory bases and classify the
  shared authority shape.
- Consume explicit string constant identity, label, extent, offset, width, and
  memory-use facts when they are already present.
- Preserve fail-closed diagnostics when string identity, data extent, access
  width, address space, or selected local-memory authority is missing.
- Add focused tests that prove the policy is semantic and shared across more
  than one string-constant row shape.

## Out Of Scope

- Generic frame-slot local-memory support already closed by idea 614.
- Direct global-symbol local-memory policy covered by idea 631.
- BIR string/data producer repair unless refreshed diagnostics prove this idea
  cannot proceed without a separate producer split.
- Pointer arithmetic policy, ABI, runtime/library policy, expectations,
  unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe identifies a string-constant local-memory family with
  shared authority rather than a named-case-only target.
- At least one string-constant local-memory row moves past the current
  `unsupported_local_memory_access` owner, or is reclassified to a more precise
  producer/policy owner with current diagnostics.
- Negative proof keeps missing string data authority, global-symbol rows,
  frame-slot-only rows, aggregate homes, unsupported widths, and unrelated
  owners outside this idea.

## Closure Notes

Closed on 2026-07-09 after Step 8 reclassified the ten representative
string-constant local-memory rows with current evidence.

The accepted evidence is:

- `build/agent_state/630_step8_string_constant.log`
- `build/agent_state/630_step8_classification.md`
- `build/agent_state/630_step8_string_label_pointer_rows.tsv`
- `build/agent_state/630_step8_prepared_access_extract.txt`

All ten representative rows now publish
`layout_authority=string_constant_label_pointer` for the prior short-string
pointer-materialization shape. No row still stops at
`unsupported_local_memory_access`, and no row still shows the prior
`layout_authority=unknown range_verdict=proven_out_of_bounds` blocker for that
string-label pointer access.

Rows that consume `StringConstantLabelPointer` and pass:

- `src/20010123-1.c`
- `src/20030920-1.c`
- `src/pr35800.c`

Rows that consume `StringConstantLabelPointer` but now belong to out-of-scope
prepared move-bundle fan-in:

- `src/20011109-2.c`
- `src/20021204-1.c`
- `src/920429-1.c`
- `src/930429-1.c`
- `src/pr34415.c`
- `src/ptr-arith-1.c`

`src/20000722-1.c` consumes `StringConstantLabelPointer` and reaches object,
link, and runtime before failing with
`[RV64_BACKEND_RUNTIME_MISMATCH]` / c4c segfault. That is runtime/object
correctness work, not string-constant local-memory admission.

Follow-up split ideas created from the remaining out-of-scope owners:

- `ideas/open/637_prepared_stack_destination_fan_in_authority_producer.md`
- `ideas/open/638_rv64_string_label_pointer_runtime_object_correctness.md`

The close gate used matching focused backend CTest logs:
`test_before.log` and `test_after.log` both ran
`ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$'`.
The c4c regression guard passed in non-decreasing mode with 2/2 tests passing
before and after and no new failures.

## Reviewer Reject Signals

- Reject testcase-shaped branches for any representative source file named in
  this idea.
- Reject treating string constants as frame slots or ordinary globals without
  explicit string/data authority.
- Reject RV64 inference of string labels, extents, offsets, or payload bytes
  from source filenames, final assembly shape, or literal spelling alone.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave the same
  string-constant local-memory authority gap.
