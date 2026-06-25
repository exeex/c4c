# RV64 Object Route ABI and Width Edge Coverage

Status: Open
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair the smaller but semantically important RV64 prepared-object edge
buckets that remain after the main architecture and data-section work:
general call shapes, non-i32 local memory widths, variadic/vararg entries,
declaration control-flow entries, FPR ABI values, and unusual local-memory
homes.

## Why This Exists

The prepared-shape classification found `24` cases outside the two dominant
families:

- `11` general call lowering shape
- `6` non-i32 or pointer local memory width
- `3` variadic or vararg entry/call shape
- `2` declaration control-flow entries
- `1` floating-point or FPR ABI value
- `1` local memory addressing/home shape

These are not enough to drive the architecture alone, but they are real
backend coverage requirements once the object route can handle multi-block
text and module data.

## In Scope

- Handle declaration/control-flow entries without attempting to emit a function
  body for undefined declarations.
- Extend local memory load/store width handling for i8/i16/i64/pointer homes
  according to prepared memory metadata.
- Add targeted RV64 ABI lowering for simple FPR and variadic/vararg shapes
  that prepared BIR already models.
- Improve general call-shape handling without weakening call correctness.
- Add representative tests and classify any residual unsupported shape with
  structured diagnostics.

## Out of Scope

- Replacing the whole object-route architecture; that belongs to
  `ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md`.
- Data-section/global/string support; that belongs to
  `ideas/open/357_rv64_object_route_data_sections_globals_strings.md`.
- Full vector ABI support.
- Passing all GCC torture cases by expectation changes.

## Representative Cases

- `src/20010119-1.c`: general call lowering shape.
- `src/20001203-1.c`: i64/i8 local stack memory stores.
- `src/20030914-2.c`: byval/vararg-adjacent aggregate entry shape.
- `src/920908-1.c`: vararg family.
- `src/20030216-1.c`: declaration control-flow entry.
- `src/20030125-1.c`: FPR/floating-point value shape.
- `src/920410-1.c`: local memory addressing/home edge.

## Suspected Stage

RV64 prepared object emission, ABI lowering, local memory emission, and
prepared control-flow admission.

## Proof Command

Narrow proof:

```sh
CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-abi-width-allowlist.txt \
  scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

with:

```text
src/20010119-1.c
src/20001203-1.c
src/20030914-2.c
src/920908-1.c
src/20030216-1.c
src/20030125-1.c
src/920410-1.c
```

Milestone proof:

```sh
CASE_TIMEOUT_SEC=20 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

## Acceptance Criteria

- Each representative case either executes correctly through clang link and
  qemu or moves to a later structured unsupported bucket.
- Local memory widths are selected from semantic/prepared memory metadata, not
  testcase names.
- Undefined declarations are represented as symbols/declarations rather than
  emitted as empty text functions.
- Any vararg/FPR support follows RV64 ABI facts intentionally documented in the
  implementation or tests.

## Reviewer Reject Signals

- Reject if the implementation hard-codes listed testcase names or source
  patterns.
- Reject if vararg/FPR behavior is guessed without reference to prepared ABI
  metadata.
- Reject if non-i32 local memory is handled by truncating/widening silently
  instead of emitting correct load/store width semantics.
- Reject if declaration handling drops needed undefined symbols.
- Reject if scan progress is claimed from skipped or weakened runtime checks.

