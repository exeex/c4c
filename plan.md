# RV64 String-Label Pointer Runtime Object Correctness Runbook

Status: Active
Source Idea: ideas/open/638_rv64_string_label_pointer_runtime_object_correctness.md

## Purpose

Classify the first concrete owner of the `src/20000722-1.c` runtime/object
failure after string-label pointer authority has already been consumed.

## Goal

Produce evidence that routes the row to one owner: ABI, layout, local/global
memory, call lowering, branch/control flow, true runtime support, or
unresolved with explicit missing evidence.

## Core Rule

Do not implement a runtime fix or reopen string-constant local-memory
admission. This runbook is evidence and classification work only.

## Read First

- `ideas/open/638_rv64_string_label_pointer_runtime_object_correctness.md`
- `ideas/closed/630_string_constant_local_memory_policy.md`
- `ideas/closed/618_runtime_mismatch_ownership_investigation.md`
- `docs/runtime_mismatch_ownership/03_followup_implementation_queue.md`

## Current Targets

- Representative row: `src/20000722-1.c`
- Current proof surface: object/link success followed by
  `[RV64_BACKEND_RUNTIME_MISMATCH]` / c4c segfault
- Important prior fact: object emission already consumes
  `layout_authority=string_constant_label_pointer`

## Non-Goals

- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime comparison, pass/fail accounting, or test contracts.
- Do not broaden `StringConstantLabelPointer` or reopen string-constant
  local-memory admission from idea 630.
- Do not group unrelated segfault rows under a generic runtime-support bucket.
- Do not make implementation edits in this research route.

## Working Model

The row is past the string-label pointer admission blocker. The next packet
must identify the first owner of the remaining object/runtime failure using
fresh artifacts rather than guessing from the segfault symptom.

## Execution Rules

- Preserve raw object, link, disassembly, and runtime evidence in the packet
  notes or a focused docs artifact if the supervisor asks for one.
- Prefer current diagnostics and artifact comparison over source-file names or
  final symptom labels.
- If evidence proves a single implementation owner, recommend a separate open
  implementation idea instead of widening this research plan.
- Keep `todo.md` as the live packet state; update this runbook only if the
  evidence route itself changes.

## Ordered Steps

### Step 1: Refresh `src/20000722-1.c` object/link/runtime evidence

Goal: Reproduce the current first failure from a fresh build.

Primary target: `src/20000722-1.c`

Actions:

- Run the supervisor-selected build and narrow proof command for the row.
- Capture the object emission, link status, disassembly, and runtime result.
- Confirm the row still reaches object/link before the runtime mismatch or
  segfault.
- Confirm the string-label pointer authority fact remains consumed before
  object emission.

Completion check:

- The packet records the exact current failure symptom and artifacts needed to
  compare ownership lanes.

### Step 2: Classify the first owner

Goal: Determine whether the first bad fact is ABI, layout, local/global memory,
call lowering, branch/control flow, true runtime support, or unresolved.

Primary target: evidence captured in Step 1

Actions:

- Compare the row against the runtime mismatch ownership lanes from idea 618.
- Inspect whether string-label address materialization is correct before
  runtime.
- Inspect local/global memory lowering, stack layout, ABI/call setup, and
  branch/control-flow evidence in that order.
- Record concrete positive and negative evidence for each rejected owner.

Completion check:

- `todo.md` names one first owner with supporting artifacts, or explicitly
  marks the row unresolved with the missing evidence needed to decide.

### Step 3: Split or recommend the implementation follow-up

Goal: Convert a proven owner into a focused next initiative without
implementing it here.

Primary target: the owner classification from Step 2

Actions:

- If one implementation owner is proven, draft or recommend a focused follow-up
  idea with one proof surface.
- If evidence points to multiple independent owners, split them instead of
  creating one broad runtime bucket.
- If the row is unresolved, record the next evidence command or artifact needed
  before implementation can begin.

Completion check:

- The route either identifies a focused follow-up owner or records why no
  implementation idea is yet justified.

### Step 4: Research acceptance review

Goal: Decide whether idea 638 is complete as research.

Primary target: plan/todo evidence and any created research artifact

Actions:

- Verify no implementation files, expectations, allowlists, unsupported
  markers, timeout/accounting, or runtime-comparison contracts were changed as
  claimed progress.
- Verify the selected owner is backed by artifacts rather than only the
  runtime mismatch label.
- Verify any follow-up recommendation is single-owner and proof-surface
  focused.

Completion check:

- The supervisor has enough evidence to ask the plan owner to close this
  research idea or to rewrite/split the route with a concrete blocker.
