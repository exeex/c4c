# RV64 Object Route Local Memory Addressing Edges Runbook

Status: Active
Source Idea: ideas/open/400_rv64_object_route_local_memory_addressing_edges.md

## Purpose

Repair prepared RV64 object-route local-memory base, offset, width, and
frame-slot access forms without weakening the prepared-object contract.

## Goal

Classify the current local-memory addressing failures, implement reusable RV64
object lowering only for valid prepared facts, and split producer-side memory
fact gaps when the handoff is insufficient.

## Core Rule

Lower prepared local-memory facts semantically. Do not infer stack layout,
aliasing, aggregate storage, or source-level union shape inside RV64 object
emission when those facts are absent or contradictory.

## Read First

- `ideas/open/400_rv64_object_route_local_memory_addressing_edges.md`
- `todo.md`
- `tests/c/external/gcc_torture/src/20000722-1.c`
- `tests/c/external/gcc_torture/src/20030910-1.c`
- `tests/c/external/gcc_torture/src/20020225-2.c`
- RV64 object emission diagnostics around `unsupported_local_memory_access`
- Prepared local-memory, frame-slot, and memory-audit code that publishes
  base-plus-offset, access size, alignment, and range facts
- RV64 backend runner `scripts/check_progress_rv64_gcc_c_torture_backend.sh`

## Current Scope

- Classify local-memory base kinds, offsets, access widths, alignments, and
  range verdicts in the current object-route bucket.
- Include the inherited `src/20020225-2.c` failure after idea 401: it now
  reaches `unsupported_local_memory_access`, not `unsupported_floating_cast`.
- Add lowering for reusable prepared local-memory forms that are valid under
  the prepared contract.
- Split producer-side missing or contradictory frame-slot/local-memory facts
  into a separate idea instead of compensating in RV64 emission.

## Non-Goals

- Do not reopen scalar compare/trunc or floating-cast lowering from idea 401.
- Do not implement scalar `ashr` or bitfield-global instruction fragments here.
- Do not repair wide rematerialized-immediate producer admission here.
- Do not treat aggregate, union, compound-literal, or representative filenames
  as named special cases.
- Do not weaken local-memory access-width diagnostics or expectations without
  adding real lowering or a precise producer split.

## Working Model

- The object route already admits only selected prepared local-memory accesses;
  the next work must distinguish valid missing target lowering from invalid or
  incomplete prepared facts.
- The `src/20020225-2.c` inherited blocker has an 8-byte `double` store from an
  FPR cast producer into local union storage, while the prepared stack model
  reports one-byte fixed slots and an out-of-bounds range verdict. That may be
  a producer/layout fact gap rather than a target-emission gap.
- Existing representatives `src/20000722-1.c` and `src/20030910-1.c` remain
  the primary local-memory bucket probes from the source idea.

## Execution Rules

- Start with classification before code changes; record representative facts
  and exact diagnostics in `todo.md`.
- Preserve the prepared-object boundary. RV64 emission may lower explicit
  facts, but must not invent base, offset, size, alias, or stack-layout facts.
- Keep target lowering and producer fact repair as separate packets unless the
  same helper contract genuinely covers both.
- Each code-changing packet needs the supervisor-delegated build/proof command
  recorded in `test_after.log` and `todo.md`.
- Prove representative and nearby same-bucket cases, not only one inherited
  testcase.

## Ordered Steps

### Step 1: Classify Local-Memory Addressing Forms

Goal: identify the reusable prepared local-memory forms and separate target
lowering gaps from producer fact gaps.

Primary targets:

- `tests/c/external/gcc_torture/src/20000722-1.c`
- `tests/c/external/gcc_torture/src/20030910-1.c`
- `tests/c/external/gcc_torture/src/20020225-2.c`
- RV64 object-route diagnostics for local-memory base, offset, width, and
  range failures

Actions:

- Run or inspect a supervisor-selected allowlist probe for local-memory
  representatives.
- Capture prepared facts for base kind, offset, size, alignment, frame slot,
  source value home, and range verdict.
- Decide whether the first implementation packet is valid target lowering or a
  producer-side prepared fact split.
- Record the chosen proof command and same-bucket neighbor cases in `todo.md`.

Completion check:

- `todo.md` records the exact local-memory facts, diagnostic evidence, selected
  first packet, and proof command without claiming implementation progress.

### Step 2: Lower Valid Prepared Local-Memory Forms

Goal: add RV64 object lowering for reusable local-memory access forms whose
prepared facts are complete and internally consistent.

Primary targets:

- RV64 object emission local-memory load/store lowering
- Prepared frame-slot or pointer-value base-plus-offset access facts
- Representative local-memory cases from Step 1

Actions:

- Implement semantic lowering for valid prepared base-plus-offset and width
  families identified in Step 1.
- Preserve access size, alignment, source/destination register bank, and
  frame-slot range semantics.
- Avoid matching on filenames, function names, aggregate shapes, constants, or
  one exact instruction sequence.
- Run the delegated build/proof command and record before/after diagnostic
  movement in `todo.md`.

Completion check:

- Representative valid local-memory cases no longer fail with the same
  object-route local-memory diagnostic, or `todo.md` names the exact producer
  fact split required before target lowering can proceed.

### Step 3: Split Producer-Side Memory Fact Gaps

Goal: route missing or contradictory prepared local-memory facts to the correct
source idea instead of patching around them in RV64 emission.

Primary targets:

- Prepared local-memory fact publication
- Frame-slot layout and range verdict producers
- Aggregate/union local storage cases that lack coherent prepared facts

Actions:

- For any representative with incomplete or contradictory facts, identify the
  producer boundary precisely.
- Reuse an existing open idea when it already owns the producer/fact family.
- Create a new source idea only when no existing open idea owns the distinct
  producer work, including concrete acceptance and reviewer reject signals.
- Keep `todo.md` aligned with the active 400 plan if target lowering remains
  the next packet; otherwise return to the supervisor for a lifecycle switch.

Completion check:

- Remaining non-target local-memory blockers are represented in the right open
  idea and are not silently absorbed into this RV64 emission runbook.

### Step 4: Refresh Local-Memory Bucket Proof

Goal: prove the local-memory bucket moved in the intended direction and leave
clean lifecycle state for any distinct follow-up.

Primary targets:

- `todo.md`
- `test_after.log`
- RV64 gcc_torture backend subset output

Actions:

- Run the supervisor-selected refreshed subset or backend progress probe.
- Confirm the local-memory object-route bucket count decreases without new
  runtime mismatches.
- Record remaining distinct diagnostics in `todo.md`.
- If all acceptance criteria are satisfied, ask the plan owner to close the
  idea; otherwise route separate producer or target-emission initiatives
  through open ideas.

Completion check:

- Canonical lifecycle state says whether idea 400 is complete, still active for
  a remaining local-memory packet, or blocked by a distinct follow-up.
