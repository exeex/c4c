# RV64 Object Route ABI and Width Edge Coverage

Status: Open
Type: Repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Route and repair the smaller but semantically important prepared-object edge
buckets after the shared BIR/prepared consumer contract is complete. This idea
must split edge buckets by layer instead of treating all of them as RV64 object
emission work.

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

Layer routing after idea 359:

- Variadic/vararg and `va_arg` semantics are upstream LIR/BIR/prepared
  contract work, not RV64 object emission. They are split to
  `ideas/closed/360_lir_bir_vararg_va_arg_contract_completion.md`; remaining
  RV64 target hook/materialization work is tracked by
  `ideas/open/361_rv64_vararg_abi_hook_materialization.md`.
- Non-i32/pointer local memory width, unusual local-memory homes, and
  declaration-control-flow entries depend on the shared consumer contract from
  359 before target emission should repair them.
- FPR/simple call ABI may remain a later target ABI emission follow-up only
  where BIR/prepared already models the required semantics.

## In Scope

- Handle declaration/control-flow entries without attempting to emit a function
  body for undefined declarations, after 359 defines the prepared traversal and
  declaration/terminator contract consumed by targets.
- Extend local memory load/store width handling for i8/i16/i64/pointer homes
  according to prepared memory metadata and value-home transfer plans supplied
  by 359.
- Add targeted RV64 ABI lowering for simple FPR and ordinary call shapes only
  where prepared BIR already models the required call/value semantics.
- Improve general call-shape handling without weakening call correctness, and
  without inventing missing call/argument semantics in target object emission.
- Add representative tests and classify any residual unsupported shape with
  structured diagnostics.

## Out of Scope

- Replacing the whole object-route architecture; that belongs to
  `ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md`.
- Completing the shared prepared object consumer contract; that belongs to
  `ideas/open/359_bir_prepared_object_consumer_contract_completion.md`.
- Data-section/global/string support; that belongs to
  `ideas/open/357_rv64_object_route_data_sections_globals_strings.md`.
- Variadic/vararg and `va_arg` LIR/BIR/prepared semantics; that belongs to
  `ideas/closed/360_lir_bir_vararg_va_arg_contract_completion.md`. Remaining
  RV64 target hook/materialization work belongs to
  `ideas/open/361_rv64_vararg_abi_hook_materialization.md`.
- Full vector ABI support.
- Passing all GCC torture cases by expectation changes.

## Representative Cases

- `src/20010119-1.c`: general call lowering shape.
- `src/20001203-1.c`: i64/i8 local stack memory stores.
- `src/20030216-1.c`: declaration control-flow entry.
- `src/20030125-1.c`: FPR/floating-point value shape.
- `src/920410-1.c`: local memory addressing/home edge.

Routed out of this mixed-edge idea:

- `src/20030914-2.c`: byval/vararg-adjacent aggregate entry shape.
- `src/920908-1.c`: vararg family.

## Suspected Stage

Mixed by bucket:

- local memory width/home and declaration entries: shared 359 consumer contract
  first, then target emission;
- simple call/FPR ABI: later target ABI emission only where prepared semantics
  already exist;
- vararg/va_arg: upstream LIR/BIR/prepared contract closed in idea 360;
  remaining RV64 target hook/materialization work lives in idea 361.

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
- Vararg/va_arg shared-contract cases are no longer treated as this mixed
  RV64 object-emission repair scope; shared contract work is closed in idea 360
  and remaining target hook work is routed to idea 361.
- Local memory widths are selected from semantic/prepared memory metadata, not
  testcase names.
- Undefined declarations are represented as symbols/declarations rather than
  emitted as empty text functions.
- Any FPR/simple call support follows RV64 ABI facts intentionally documented in
  the implementation or tests and only consumes prepared semantics already
  provided upstream.

## Reviewer Reject Signals

- Reject if the implementation hard-codes listed testcase names or source
  patterns.
- Reject if shared vararg/va_arg behavior is implemented in RV64 object
  emission instead of consuming the idea 360 contract or routing target hook
  work through idea 361.
- Reject if FPR/simple-call behavior is guessed without reference to prepared
  ABI metadata.
- Reject if non-i32 local memory is handled by truncating/widening silently
  instead of emitting correct load/store width semantics.
- Reject if declaration handling drops needed undefined symbols.
- Reject if scan progress is claimed from skipped or weakened runtime checks.
