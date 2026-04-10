# x86_64 Backend Bring-Up After BIR Cutover

Status: Open
Last Updated: 2026-04-10

## Why This Idea

The original post-BIR x86_64 bring-up lane started as a bounded recovery of
`c_testsuite_x86_backend_*` failures after legacy backend rescue paths were
removed. That framing was directionally correct, but execution drifted into a
less healthy pattern: more and more unsupported cases were recovered by adding
`try_lower_minimal_*` recognizers under the shared `lir_to_bir` boundary rather
than by growing reusable lowering rules.

The current problem is no longer just "x86_64 is missing more supported
shapes." The higher-leverage problem is that shared-BIR recovery for x86_64 has
been accumulating testcase-shaped legacy seams:

- exact `print_llvm()` / `kExpectedModule` matchers
- narrowly named `try_lower_minimal_*` routes keyed to one source shape
- duplicated aggregate and memory recognizers split by field count or testcase
  flavor rather than by semantic family
- a `try_lower_to_bir_with_options(...)` dispatcher that still returns early
  from many `.phase = "legacy-lowering"` seams before generic ownership takes
  over

That debt now directly limits further x86_64 bring-up. Continuing to recover
new source cases on top of the current matcher growth would improve pass count
short-term while making the shared lowering path harder to maintain and harder
to generalize.

## Source Context

- `ideas/closed/40_target_profile_and_execution_domain_foundation.md`
- `ideas/closed/41_bir_full_coverage_and_ir_legacy_removal.md`
- `test_x86_64_backend_baseline_20260409T140902Z.log`

## Architectural Reassessment

There is already a split lowering tree under
`src/backend/lowering/lir_to_bir/`, including:

- `cfg.cpp`
- `phi.cpp`
- `memory.cpp`
- `calls.cpp`
- `aggregates.cpp`

But the existence of those files does not mean the generic path owns lowering.
On the current tree:

- `cfg.cpp` and `phi.cpp` do perform real normalization/lowering work
- `memory.cpp`, `calls.cpp`, and `aggregates.cpp` contain useful helpers and
  matcher cleanup work, but a significant portion of their "pass" surface is
  still legacy recognition
- `record_memory_lowering_scaffold_notes(...)`,
  `record_call_lowering_scaffold_notes(...)`, and
  `record_aggregate_lowering_scaffold_notes(...)` are still mostly scaffold
  markers rather than full ownership handoff
- `try_lower_to_bir_with_options(...)` still front-loads many
  `try_lower_minimal_*` seams marked as `legacy-lowering`
- the normalized fallback still ends in `try_lower_to_bir_legacy(...)`

So the real current state is:

- formal API surface: `try_lower_to_bir(...)`
- real execution center of gravity: legacy matcher dispatch plus legacy fallback

This idea therefore needs to change emphasis. The immediate job is not "keep
adding source-backed x86 recoveries." The immediate job is "stop feeding the
legacy matcher surface so shared-BIR can become a real owner."

## Lifecycle Note

As of 2026-04-10, active execution has been switched away from this idea and
into a new x86 peephole completion lane under idea 43. This idea remains open
as the parked shared-BIR cleanup and legacy-matcher consolidation track.

When reactivated, resume from:

- family-based cleanup of the remaining `legacy-lowering` matcher surface
- consolidation of the global aggregate compare family
- reduction of dispatcher pressure in `try_lower_to_bir_with_options(...)`

## Current Failure Shape

### Bucket 1: Shared-BIR legacy matcher debt

The active `lir_to_bir` path still contains many source-shaped seams, primarily
under `memory.cpp`, `calls.cpp`, and the dispatcher in `lir_to_bir.cpp`.

Representative symptoms:

- exact rendered-LLVM text matching
- block-label-sensitive ladders
- helpers named by testcase flavor such as
  `global_named_two_field_struct_designated_init_compare_zero_return`
- multiple adjacent matchers that should be one semantic family

This bucket is the primary blocker because it distorts where future x86_64
bring-up work lands.

### Bucket 2: x86_64 source-backed coverage still depends on shared-BIR cleanup

Many x86_64 cases that were previously recovered now pass through shared BIR,
but the current implementation technique often remains matcher-shaped. New
coverage should not continue expanding until the active shared-BIR family stops
growing in legacy debt.

### Bucket 3: Separate native/runtime/toolchain issues

Native x86 assembly coverage, runtime/variadic issues, and unrelated parser or
cross-target failures still exist, but they should remain separate from this
cleanup-first lane unless they directly block the active matcher-family
consolidation.

## Goal

Finish the x86_64 post-BIR bring-up without spending more engineering effort on
testcase-shaped shared lowering. Convert the active lane from "recover the next
source case" into "shrink the legacy matcher surface until generic ownership is
credible again," while preserving already-recovered x86_64 coverage.

## Non-Goals

- do not revive legacy backend IR
- do not restore LLVM asm rescue
- do not widen this idea into a full multi-target lowering rewrite
- do not absorb unrelated parser, EASTL, or cross-target failures
- do not keep adding new testcase-specific matchers as the default execution
  mode

## Working Model

- treat `try_lower_to_bir(...)` as the formal API surface
- treat `try_lower_to_bir_legacy(...)` and all `.phase = "legacy-lowering"`
  paths as shrink targets, not normal expansion targets
- prefer one semantic family at a time over one testcase at a time
- prefer instruction-structured matching over rendered-text matching
- preserve existing x86_64 source-backed wins while shrinking the matcher debt

## Matcher Families

The current legacy surface already groups into a few coherent families. That is
the right abstraction level for cleanup and eventual generic ownership.

### Family A: Global constant aggregate compare ladders

Representative seams:

- global anonymous struct field compare
- global named designated-init struct compare
- global int-pointer designated-init struct compare
- global nested struct / anonymous union compare
- nested anonymous aggregate alias compare

These should converge toward one family-level helper such as
`match_global_constant_aggregate_compare_ladder(...)` instead of continuing as
separate `named_two_field` / `named_three_field` style routes.

### Family B: Local aggregate and local-slot compare ladders

Representative seams:

- local struct shadow store/compare
- local single-field struct store/load
- local paired single-field compare/sub
- local enum constant compare/store/load
- related local aggregate alias and sum shapes

These should converge toward a family-level local aggregate / compare rule.

### Family C: Pointer and local-array address/alias families

Representative seams:

- local i32 pointer alias compare
- local array pointer inc/dec compare
- local array pointer add/deref/diff
- local array pointer store/compare routes

These should converge toward a reusable pointer/address lowering family rather
than per-shape recognizers.

### Family D: String and char compare ladders

Representative seams:

- local string-literal char compare ladder
- normalized string compare + phi return seam

These should converge toward one string-backed compare-chain rule instead of
split pre-normalize and post-normalize one-offs.

### Family E: Helper-call and bounded call seams

Representative seams:

- repeated `printf` local-i32 route
- local array pointer alias + `sizeof` helper call
- local char helper call with dead-array compare

These should converge toward reusable bounded call / side-effect ownership, not
case-by-case call matchers.

## Execution Stance

This idea is now explicitly cleanup-first.

That means the near-term work should:

- remove exact rendered-IR matchers
- group adjacent legacy matchers by family
- replace testcase-flavored names and ownership with family-level helpers where
  feasible
- reduce dispatcher pressure in `try_lower_to_bir_with_options(...)`
- only resume new x86_64 source-backed case recovery after the active matcher
  family stops growing

This is intentionally different from the earlier "pick the next failing x86
testcase" mode. The tree has reached the point where architecture cleanup is
the fastest path to sustainable coverage.

## Proposed Plan

### Step 1: Inventory the remaining legacy matcher surface

Goal: make the remaining matcher debt explicit by file, family, and risk.

Concrete actions:

- inventory exact-text and strongly testcase-shaped matchers in
  `lir_to_bir.cpp`, `memory.cpp`, and `calls.cpp`
- group them into family-level buckets instead of testcase labels
- identify which families already have partial helpers and which still only
  exist as one-off routes

Completion check:

- the remaining cleanup queue is short, prioritized, and family-based

### Step 2: Remove low-risk exact-text seams family by family

Goal: stop the most brittle matcher form from spreading further.

Concrete actions:

- convert one family slice at a time from rendered-text to structured matching
- add narrow regression tests that prove the cleanup is no longer coupled to
  SSA temp names or exact rendered snippets
- keep the work bounded to the active family instead of expanding to unrelated
  x86 cases

Completion check:

- the chosen family slice no longer depends on full rendered LLVM text
- already-owned x86_64 route coverage for that slice still passes

### Step 3: Consolidate family-level ownership and reduce legacy dispatcher pressure

Goal: start leaving the worst `try_lower_minimal_*` explosion behind.

Concrete actions:

- merge adjacent matchers that are really one semantic family
- move common reasoning into family helpers instead of duplicating by testcase
  flavor
- shrink the visible `legacy-lowering` surface in
  `try_lower_to_bir_with_options(...)`

Completion check:

- at least one family is represented by fewer seams than before
- the active queue is no longer framed as "add the next bounded matcher"

### Step 4: Resume x86_64 failing-case recovery only after cleanup turns the corner

Goal: resume coverage growth from a healthier shared-BIR base.

Concrete actions:

- use the cleaned-up family surface to reassess the highest-leverage remaining
  x86_64 failures
- keep native-runtime or cross-target regressions separated into their own
  lifecycle items when needed

Completion check:

- new x86_64 case recovery resumes from a smaller legacy surface, not from a
  larger one
