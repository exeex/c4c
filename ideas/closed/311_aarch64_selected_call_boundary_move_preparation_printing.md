# AArch64 Selected Call-Boundary Move Preparation Printing

Status: Closed
Created: 2026-05-19
Closed: 2026-05-19
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Closure Notes

The focused owner is complete. Recent implementation slices repaired AArch64
stack call-argument destination offset publication, selected scalar
frame-slot-to-stack call-boundary moves, aggregate/byval stack-copy lowering
from prepared source addresses into outgoing call stack slots, and machine
printer coverage for the repaired selected-node shapes.

Close proof used the supervisor-selected focused scope:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00140_c)$'
```

`test_after.log` records all four focused tests passing, including
`c_testsuite_aarch64_backend_src_00140_c`. The old selected call-boundary move
`DeferredUnsupported` diagnostic and the later aggregate stack-copy residual
are both gone. Close-time regression guard over the same focused scope passed
with non-decreasing results and no new failures.

## Goal

Repair the AArch64 call-boundary move path so prepared move records are marked
selected only when they carry printable prepared source and destination facts,
and so selected records print through the machine printer without falling back
to `DeferredUnsupported`.

## Why This Exists

Umbrella idea 295 Step 3 found a focused compile-stage residual in the fresh
`test_after.log`: `c_testsuite_aarch64_backend_src_00140_c` fails with
`FRONTEND_FAIL` at AArch64 machine-node printing:

```text
printer requires selected machine node, got deferred_unsupported: call-boundary move node is outside the selected register call-boundary move subset
```

No `build/c_testsuite_aarch64_backend/src/00140.c.s` artifact exists, which
means the route stops before assembly emission. The semantic owner is selected
call-boundary move preparation and printer admission, not instruction spelling.

`00140.c` exercises direct calls involving a by-value aggregate, pointer
arguments, and a variadic call. That source shape is useful pressure on
call-boundary move preparation, but the repair must be expressed through
prepared value homes, call argument plans, ABI bindings, and value-move bundle
facts rather than filename, argument-index, or `struct foo` matching.

## In Scope

- Inspect AArch64 call-boundary move record construction and selection status
  for prepared register/register and related prepared move forms.
- Preserve prepared source and destination facts from value homes, call
  argument plans, ABI bindings, and value-move bundles before a
  call-boundary move can be selected.
- Repair selected-node admission so supported call-boundary moves become
  selected only with enough structured data for the printer.
- Keep fail-closed diagnostics for unsupported, unprepared, or incomplete
  call-boundary move records.
- Add or update focused backend tests in:
  - `backend_aarch64_target_instruction_records`
  - `backend_aarch64_machine_printer`
  - `backend_aarch64_instruction_dispatch`
- Use `c_testsuite_aarch64_backend_src_00140_c` as the focused external
  compile/printer proof after backend coverage identifies the semantic record
  shape.

## Out of Scope

- Expectation, allowlist, unsupported-classification, runner, timeout-policy,
  proof-log, CTest-registration, or test-contract changes.
- Suppressing the diagnostic or bypassing the selected-machine-node printer
  gate.
- Marking a call-boundary move selected without a printable prepared source
  and destination.
- Filename-only, argument-index-only, `struct foo`-only, source-shape-only, or
  emitted-mnemonic-only fixes.
- Reopening closed owners 285 through 310 without generated-code or proof
  evidence that contradicts their closure boundaries.
- Direct multi-argument shuffle, direct vararg aliasing, address-of-local call
  argument preparation, runtime nonzero/mismatch/crash, timeout/output-storm,
  other machine-printer residuals such as `00164.c` and `00214.c`, or semantic
  `lir_to_bir` residuals parked under umbrella idea 295.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00140_c` no longer fails with the old
  selected call-boundary move `DeferredUnsupported` printer diagnostic.
- Backend tests prove the prepared call-boundary move record carries printable
  source and destination facts before it is marked selected.
- The machine printer still rejects unselected or incomplete call-boundary
  move records with fail-closed diagnostics.
- The repair is semantic across selected call-boundary move preparation and
  printing, not a named c-testsuite shortcut.
- Fresh build proof is recorded for the implementation slice.
- Supervisor-selected proof includes:

```bash
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00140_c)$'
```

## Reviewer Reject Signals

Reject the route if it:

- fixes only `00140.c`, `struct foo`, one argument index, one source spelling,
  or one emitted mnemonic instead of repairing selected call-boundary move
  preparation and printable source/destination publication;
- claims progress through expectation rewrites, allowlist changes,
  unsupported-classification changes, CTest registration, runner behavior,
  timeout policy, proof-log edits, or weaker test contracts;
- suppresses or rewrites the diagnostic while the same call-boundary move
  remains outside the selected register subset;
- marks a call-boundary move selected without proving a printable prepared
  source and destination exist;
- removes fail-closed machine-printer behavior for unsupported or incomplete
  call-boundary move records;
- broadens into direct-call shuffle, direct vararg, address-of-local,
  runtime, timeout, unrelated machine-printer, or `lir_to_bir` residual
  buckets without separate split evidence;
- claims capability progress from helper renames, classification-only edits,
  or diagnostic text changes while the old unprintable selected-node handoff
  remains behind a new abstraction name.
