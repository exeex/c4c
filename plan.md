# AArch64 Edge Terminator Consumer Preservation Repair Runbook

Status: Active
Source Idea: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Activated After: closed idea 52

## Purpose

Resume the parked edge/terminator consumer-preservation repair now that the
call/byval publication owner has been closed, and judge the edge-preservation
contract on its own source-idea criteria.

## Goal

Repair prepared edge and terminator publication so successor consumers can use
the correct predecessor value even when out-of-SSA edge publication reuses the
same physical register for another incoming value.

## Core Rule

Fix the prepared placement and consumption contract. Do not repair the route
with testcase names, join-time reloads from mutated locals, textual assembly
suppression, or AArch64-only instruction shuffling that leaves prepared
planning unable to describe the preservation requirement.

## Read First

- `ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md`
- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

## Current Scope

- Prepared consumer-move placement that depends on predecessor terminator or
  edge-publication clobbers.
- Out-of-SSA edge publication ordering when a source register for a later
  successor consumer is reused for another incoming value.
- Shared prepared lookup facts needed by AArch64 lowering to consume preserved
  sources without raw BIR scans.
- Minimal AArch64 consumer changes only when they consume the prepared
  preservation answer.

## Non-Goals

- Do not reload `%lv.ap.24` in the join block after the va_list field has been
  mutated.
- Do not add cast-only or ALU-only stale-register fallbacks without providing a
  valid preserved source.
- Do not change AAPCS64 call staging, variadic layout constants, or va_list
  cursor writeback rules.
- Do not fold unrelated dispatch value-materialization, memory identity,
  comparison, global-load, or broad calls lowering repairs into this route.
- Do not weaken expectations or mark supported probes unsupported.

## Working Model

The target failure family is a prepared-placement bug: a successor consumer
needs a predecessor value after edge publication would clobber the physical
register that still appears to hold the source. Prepared planning must either
place the consumer move before predecessor edge publication or expose a
preserved source home that successor lowering can consume.

## Execution Rules

- Treat the `%t35 -> %t45.byte_offset.i64` case as a diagnostic example only.
  The implementation must be value- and block-agnostic.
- Prefer prepared/regalloc facts over raw BIR scans or name matching.
- Keep AArch64 edits limited to consuming the prepared preservation result.
- After each code-changing step, run a fresh build or compile proof plus the
  supervisor-selected focused subset.
- Escalate to broader backend validation when a step changes shared prealloc
  placement or publication-plan behavior.

## Ordered Steps

### Step 1: Re-establish the Edge-Preservation Failure Boundary

Goal: Confirm what remains after idea 52 closed and identify whether the
edge/terminator preservation acceptance criteria can now be judged directly.

Primary target: focused backend/runtime probes selected by the supervisor.

Actions:

- Inspect current prepared consumer-move and edge-publication behavior around
  successor consumers whose predecessor source register is clobbered.
- Re-run or request the focused eight-test subset referenced by the source
  idea, including the pointer-select and variadic aggregate byte-copy probes.
- Record whether any remaining failure is still an edge/terminator
  preservation failure or belongs to a different owner.

Completion check:

- The current first bad fact is classified as either this idea's preservation
  gap or a precise non-owned follow-up before implementation proceeds.

### Step 2: Repair Prepared Preservation Placement

Goal: Make prepared planning represent sources that must survive predecessor
edge publication or terminator-side clobbering.

Primary targets:

- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`

Actions:

- Find where successor consumer moves are placed relative to predecessor
  terminator and edge-publication moves.
- Add or repair the general rule that detects when edge publication reuses a
  source register needed by a later successor consumer.
- Represent the result as prepared placement or preserved-source data, not as a
  special case for one block, value, register, or testcase.

Completion check:

- Prepared planning can explain that the affected source is moved or preserved
  before the clobbering edge publication, and nearby same-feature cases use the
  same rule.

### Step 3: Expose And Consume The Prepared Preservation Answer

Goal: Ensure AArch64 lowering consumes the repaired preservation contract
instead of trusting stale emitted-register mappings.

Primary targets:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

Actions:

- Add a narrow shared lookup only if existing prepared structures cannot answer
  which preserved source or placement applies.
- Update AArch64 edge-copy lowering to consume the prepared answer while
  preserving existing instruction spelling and edge-copy mechanics.
- Avoid raw BIR scans, same-block producer walks, temporary-name checks, or
  textual suppression of the bad assembly sequence.

Completion check:

- AArch64 lowering no longer uses stale register authority after edge
  publication changes the physical register's semantic owner.

### Step 4: Prove The Focused Preservation Family

Goal: Demonstrate the repaired prepared contract covers the known focused
family without downgrading expectations.

Primary target: supervisor-selected focused subset for the source idea.

Actions:

- Run the exact build and focused proof command delegated by the supervisor.
- Include the pointer-select and variadic aggregate byte-copy probes named by
  the source idea.
- Inspect any failure for route ownership before making more changes.

Completion check:

- The focused subset passes, or any remaining failure is classified to a
  precise next owner with evidence that edge/terminator preservation is no
  longer the first bad fact.

### Step 5: Broader Regression Check And Closure Readiness

Goal: Establish whether the repaired shared prealloc behavior is safe enough
for supervisor review and possible source-idea closure.

Primary target: repo-native broader backend validation chosen by the
supervisor.

Actions:

- Run the supervisor-selected broader check after the focused subset is green
  or the route is otherwise ready for review.
- Compare the implementation against the source idea's reviewer reject signals.
- Leave any remaining non-owned work in `todo.md` or a separate open idea
  rather than expanding this route.

Completion check:

- Fresh proof exists, no expectation downgrade or testcase-shaped shortcut is
  present, and the supervisor has enough evidence to review or delegate
  closure judgment.
