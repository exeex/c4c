# RV64 C-Testsuite QEMU Nonzero Runtime Triage Runbook

Status: Active
Source Idea: ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md

## Purpose

Turn the current RV64 c-testsuite runtime failures into a concrete repair map
for later backend implementation work.

## Goal

Classify all current `QEMU_NONZERO` and timeout cases by root cause, collect
representative backend evidence, and create focused follow-up ideas for the
highest-value repair families.

## Core Rule

This is triage and planning work only. Do not implement runtime fixes, weaken
tests, downgrade supported paths, or convert runtime failures into unsupported
contracts to improve counts.

## Read First

- Source idea: `ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md`
- Latest probe, if present:
  - `build/rv64_c_testsuite_probe_latest/summary.tsv`
  - `build/rv64_c_testsuite_probe_latest/asm/`
  - `build/rv64_c_testsuite_probe_latest/bin/`
  - `build/rv64_c_testsuite_probe_latest/*.qemu.out`
  - `build/rv64_c_testsuite_probe_latest/*.emit.out`
  - `build/rv64_c_testsuite_probe_latest/*.clang.out`

## Current Targets

- The 220 allowlisted RV64 c-testsuite cases from the latest non-blocking
  probe.
- All `QEMU_NONZERO` and `QEMU_TIMEOUT` cases from that probe.
- Nearby non-runtime buckets only as context:
  - unsupported prepared global storage layouts;
  - undefined temporary-label assembler/link failures;
  - semantic `lir_to_bir` handoff rejects.

## Non-Goals

- Do not implement backend/compiler repairs under this idea.
- Do not register the full 220-case sweep as mandatory pass/fail CTest
  coverage.
- Do not treat all illegal instructions or all segfaults as one bucket without
  checking emitted patterns.
- Do not reopen closed undefined-main, string-literal, or aggregate-storage
  ideas unless evidence proves they are the current root cause for a runtime
  family.
- Do not use source filename clusters as substitutes for emitted-code or BIR
  mechanism analysis.

## Working Model

- Start from the latest probe artifacts when they are present and credible.
- Regenerate a fresh non-blocking probe only if artifacts are missing or stale.
- Classify by observed runtime outcome first, then by backend mechanism.
- Prefer semantic mechanism buckets over filename-shaped buckets.
- Follow-up ideas should be narrow enough for the next executor to start with a
  focused repair instead of repeating the whole sweep.

## Execution Rules

- Keep large generated lists and scratch artifacts under `build/` or another
  non-root scratch location.
- Keep durable idea text concise; create follow-up idea files only for
  distinct repair initiatives.
- When proof commands are needed for observation, use the probe shape from the
  source idea:

```sh
cmake --build --preset default
./build/c4cll --codegen asm --target riscv64-linux-gnu <case> -o <case>.s
/usr/bin/clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler <case>.s -o <case>.bin -lm
/usr/bin/qemu-riscv64 -L /usr/riscv64-linux-gnu <case>.bin
```

- The supervisor chooses the exact executor proof command for each packet.
- Treat the full c-testsuite sweep as observation evidence, not a required
  regression suite.

## Ordered Steps

### Step 1: Normalize Latest Probe Result

Goal: Establish the current observed counts and artifact provenance.

Primary target: `build/rv64_c_testsuite_probe_latest/summary.tsv`

Actions:

- Inspect the latest probe artifacts and decide whether they are usable.
- If needed, regenerate the same non-blocking 220-case probe shape.
- Ignore allowlist comments when counting cases.
- Record counts for `PASS`, `EMIT_FAIL`, `CLANG_FAIL`, `QEMU_NONZERO`, and
  `QEMU_TIMEOUT`.
- Verify and record that the undefined-main count remains zero.
- Keep the count provenance reproducible by naming the artifact path or command
  used.

Completion check:

- `todo.md` records the count table, provenance, and undefined-main result.

### Step 2: Split Runtime Failures By Exit Class

Goal: Separate runtime failures into initial observable groups.

Primary target: the latest probe summary and per-case qemu output files.

Actions:

- List or summarize every `QEMU_NONZERO` and timeout case.
- Split `QEMU_NONZERO` first by exit status:
  - 132 illegal instruction;
  - 139 segmentation fault;
  - other nonzero exits.
- Keep the timeout case separate from nonzero exits.
- Preserve representative case names for each exit class.

Completion check:

- `todo.md` records reproducible exit-status buckets covering all
  `QEMU_NONZERO` and timeout cases.

### Step 3: Capture Representative Backend Evidence

Goal: Collect enough evidence to classify mechanisms, not just symptoms.

Primary target: representative cases from each exit-status bucket.

Actions:

- For each meaningful exit-status group, choose a small representative sample.
- For each representative, capture:
  - source-level behavior summary;
  - prepared BIR function/control-flow summary when useful;
  - assembly around calls, branches, stack frame setup, global or pointer
    materialization, and fault-adjacent instructions;
  - qemu outcome.
- Include at least the representative illegal-instruction, segfault, non-132
  nonzero, and timeout cases identified by the source idea unless evidence
  shows better representatives.

Completion check:

- `todo.md` or scratch artifacts under `build/` contain representative evidence
  for every non-trivial runtime class.

### Step 4: Build Semantic Root-Cause Buckets

Goal: Convert raw exit classes into backend repair families.

Primary target: the evidence from Step 3.

Actions:

- Classify representatives by emitted-code or semantic mechanism, including:
  - instruction or pseudo-instruction sequence qemu cannot run;
  - bad call ABI or return-value convention;
  - bad stack frame, local slot, spill, or reload behavior;
  - bad branch or control-flow target publication;
  - bad pointer or global address materialization;
  - libc or external call interaction, including string cases that now link;
  - genuine program nonzero exits caused by wrong semantics rather than crash.
- Check whether the same mechanism explains adjacent cases before assigning
  them to a bucket.
- Leave unsupported global-storage, undefined temporary-label, and semantic
  handoff buckets separate unless tightly coupled evidence exists.

Completion check:

- Every runtime failure is listed or summarized in a reproducible semantic
  bucket, and non-runtime buckets remain visibly separate.

### Step 5: Rank Repair Families

Goal: Choose the next repair order by impact and confidence.

Primary target: semantic buckets from Step 4.

Actions:

- Estimate case impact for each bucket.
- Record confidence and representative evidence quality.
- Favor one narrow semantic fix that unlocks many nearby cases over
  case-shaped patches.
- Flag any bucket that needs deeper investigation before implementation.

Completion check:

- `todo.md` records an ordered repair recommendation with impact, confidence,
  and representative cases.

### Step 6: Create Focused Follow-Up Ideas

Goal: Preserve the next repair slices as durable source ideas.

Primary target: `ideas/open/`

Actions:

- Create at least two follow-up ideas under `ideas/open/`:
  - one for the highest-confidence illegal-instruction family;
  - one for the highest-confidence segfault, call/stack, or runtime family.
- Give each follow-up idea:
  - owned feature family;
  - candidate cases;
  - acceptance criteria;
  - reviewer reject signals against testcase-shaped fixes and expectation
    downgrades.
- Keep each idea narrow enough to be executable as a repair slice.

Completion check:

- At least two focused follow-up ideas exist under `ideas/open/`, and neither
  is a broad "make c-testsuite pass" umbrella.

### Step 7: Final Triage Check

Goal: Decide whether this source idea is ready to close.

Primary target: `plan.md`, `todo.md`, and created follow-up ideas.

Actions:

- Verify the latest 220-case counts and artifact provenance are recorded.
- Verify all runtime failures are covered by reproducible buckets.
- Verify representative evidence exists for each non-trivial bucket.
- Verify illegal instruction, segmentation fault, intentional/non-crashing
  nonzero returns, and timeout/hang behavior are explicitly separated.
- Verify at least two focused follow-up ideas were created.
- Verify generated bulk artifacts are not placed at repo root.

Completion check:

- The supervisor has enough evidence to ask the plan owner whether the active
  plan and source idea should close.
