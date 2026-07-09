# Review: 614 Step 2 Frame-Slot Local-Memory Slice

Verdict: accept

## Scope

Active source idea: `ideas/open/614_rv64_pointer_local_memory_consumption.md`

Review focus: current uncommitted Step 2 slice touching:

- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Review question: whether the slice is acceptable semantic progress under idea
614, or synthetic-only/route drift because representative C rows still stop at
`unsupported_local_memory_access`.

## Review Base

Chosen base commit: `5bfe2d162` (`[plan] Activate 614 RV64 pointer local-memory consumption`)

Why this base: it is the activation checkpoint for the active source idea and
created the current `plan.md`/`todo.md` pair for idea 614. The later commit
`f595b7bb0` is the Step 1 diagnostic packet and is relevant packet context, but
it did not reset or rewrite the active idea route.

Commit count since base: `1`

## Findings

No blocking findings.

Medium: proof is weaker than the Step 2 completion text because the named C
representatives still stop at the same broad diagnostic.

- `todo.md:34` through `todo.md:37` records that `src/20000722-1.c`,
  `src/20010123-1.c`, `src/20011109-2.c`, and `src/920429-1.c` still stop at
  `unsupported_local_memory_access`.
- Current backend case logs show the same broad rejection text for those rows,
  so this slice does not yet satisfy the runbook's stronger "multiple rows
  compile or move past their RV64 consumer stop" completion check by final-row
  outcome.
- This does not make the implementation overfit or route drift by itself,
  because `todo.md` also records the remaining first blocker as adjacent
  string-constant or pointer-value local-memory use, and recommends Step 3
  residual refresh rather than claiming those rows are fixed.

Low: the tests are synthetic, but they exercise a real prepared-authority
contract rather than a testcase-shaped shortcut.

- `tests/backend/mir/backend_riscv_object_emission_test.cpp:4060` builds a
  prepared module with explicit frame-slot id, frame layout, memory access
  address facts, byte offset, size, alignment, and `base_plus_offset`.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp:16123` preserves
  fail-closed behavior for missing frame-slot id, missing `base_plus_offset`,
  non-default address space, and unsupported 16-byte width.
- These tests should be treated as focused unit proof, not as sufficient
  end-to-end acceptance for idea 614.

Low: the implementation follows the durable source boundary.

- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:28` and
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:56` use the
  shared local-memory size helper instead of named-case matching.
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:36` through
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:44` require
  explicit default address space, non-volatile access, `FrameSlot` base,
  frame-slot id, matching size, usable base+offset, and immediate-fit offset.
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:2265` through
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:2299` and
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:2610` through
  `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp:2652` add
  width-family load/store emission without producing new pointer/address
  authority.

## Judgments

Idea alignment: matches source idea

Runbook transcription: plan matches idea

Route alignment: on track, with proof caveat

Technical debt: acceptable

Validation sufficiency: narrow proof sufficient for committing this bounded
consumer slice, but not sufficient to close or milestone idea 614

Reviewer recommendation: continue current route

## Recommendation

Accept this slice as bounded semantic progress: it consumes explicit prepared
frame-slot local-memory authority and remains fail-closed for missing authority.
Do not present it as end-to-end C-row recovery. The next packet should refresh
Step 3 residuals and either move a real row past the local-memory owner through
an adjacent selected-authority consumer, or split the remaining string-constant
/ pointer-value authority gap before more implementation.
