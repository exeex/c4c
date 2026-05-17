# Prealloc Coordinator And Fact Publishers Decomposition

Status: Closed
Created: 2026-05-17
Closed: 2026-05-17

## Intent

Split `src/backend/prealloc/prealloc.cpp` into a small coordinator plus focused
prepared-fact publisher files.

After the schema header is decomposed, `prealloc.cpp` should stop being the
home for every late prepared fact added during AArch64 bring-up. It should make
the preparation flow obvious and delegate family-specific fact construction to
files named after their responsibility.

## Why This Exists

`prealloc.cpp` currently mixes:

- generic type/register-bank helpers;
- variadic entry ABI facts;
- dynamic stack facts;
- symbol and direct-callee resolution;
- storage plans;
- call plans;
- i128/f128 carriers and runtime helper facts;
- atomic, intrinsic, and inline-asm carriers;
- frame plan publication;
- final `BirPreAlloc::run()` orchestration.

This makes the preparation pipeline hard to review. It also limits compile
parallelism because a change to one prepared fact family rebuilds the whole
coordinator.

## In Scope

- Keep `BirPreAlloc::run()` and high-level phase ordering in a small
  coordinator file.
- Extract fact-publisher implementation files such as:
  - `frame_plan.cpp`
  - `dynamic_stack.cpp`
  - `storage_plans.cpp`
  - `call_plans.cpp`
  - `variadic_plans.cpp`
  - `special_carriers.cpp`
  - `runtime_helpers.cpp`
  - `intrinsics.cpp`
  - `inline_asm.cpp`
  - `atomics.cpp`
  - `addressing.cpp`
- Share narrow private helper headers only when multiple files genuinely need
  the same helper.
- Preserve existing output and phase order.
- Keep source edits behavior-preserving and compile-proven after each
  meaningful extraction.

## Out of Scope

- Splitting `regalloc.cpp`; that is a separate idea.
- Splitting `prepared_printer.cpp`; that is a separate idea.
- Changing stack layout, liveness, regalloc, ABI, variadic, or runtime helper
  semantics.
- Redesigning `PreparedBirModule` fields.
- Moving code into target-specific AArch64 directories.

## Completion Criteria

- `prealloc.cpp` reads as the preparation coordinator rather than the owner of
  every prepared fact family.
- Fact-publisher files own the major families listed above, with names that
  match their prepared output.
- Existing tests and prepared dumps are behavior-preserving.
- No family-specific logic remains in the coordinator except for clear phase
  sequencing and shared error/note plumbing.

## Closure Notes

Closed after Step 5/Step 6 validation. `prealloc.cpp` now owns orchestration,
note emission, public preparation entry points, and publication order only.
Prepared fact construction lives in focused backend/prealloc publisher files,
with narrow duplicated or private helper ownership where extraction required
it. Final backend validation used a fresh build plus `ctest --test-dir build -j
--output-on-failure -R '^backend_'`, and the regression guard accepted equal
green before/after backend logs with 139/139 tests passing.

## Reviewer Reject Signals

Reject the route or slice if it:

- changes prepared output or test expectations while claiming ownership-only
  extraction;
- spreads one fact family across several new files without a clear owner;
- leaves the coordinator almost as large and opaque as before;
- introduces target-local AArch64 assumptions into shared prealloc;
- creates broad private helper headers that become a second monolith;
- uses testcase-shaped shortcuts to keep backend tests passing.
