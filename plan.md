# RV64 C-Testsuite Undefined-Main Triage Runbook

Status: Active
Source Idea: ideas/open/306_rv64_c_testsuite_undefined_main_triage_and_followups.md

## Purpose

Turn the RV64 c-testsuite `undefined reference to main` bucket into a
reproducible classification and a small set of focused follow-up repair ideas.

Goal: explain why the undefined-main cases happen, separate fail-closed
contract defects from unsupported RV64 feature families, and leave future
agents with concrete follow-up ideas instead of another broad investigation.

## Core Rule

This is a triage and planning runbook. Do not implement RV64 string literal,
aggregate global, floating global, pointer global, or broad c-testsuite feature
support as part of this plan.

## Read First

- `ideas/open/306_rv64_c_testsuite_undefined_main_triage_and_followups.md`
- `build/rv64_c_testsuite_probe_v2/summary.tsv`, if present
- `build/rv64_c_testsuite_probe_v2/asm/`, if present
- `build/rv64_c_testsuite_probe_v2/*.emit.out`, if present
- `build/rv64_c_testsuite_probe_v2/*.clang.out`, if present
- `build/rv64_c_testsuite_probe_v2/*.qemu.out`, if present

If probe artifacts are missing or stale, regenerate only non-blocking scratch
artifacts under `build/rv64_c_testsuite_probe_v2/`. Do not register the full
220-case sweep as required CTest coverage.

## Current Scope

- Validate or reproduce the undefined-main bucket count.
- Confirm whether representative cases still have prepared BIR containing
  `main`.
- Classify the undefined-main cases by root cause using emitted asm and
  prepared BIR evidence.
- Identify fail-closed/output-contract bugs separately from real unsupported
  feature gaps.
- Create focused follow-up ideas under `ideas/open/` for the next repair
  slices.

## Non-Goals

- Do not implement the follow-up RV64 backend features in this triage plan.
- Do not weaken existing backend expectations or accept empty `.text` as a
  successful unsupported path.
- Do not treat linker `undefined reference to main` as the root cause without
  inspecting emitted assembly and prepared BIR.
- Do not fix the known `backend_riscv_prepared_edge_publication` baseline
  failure unless a separate idea owns that repair.
- Do not leave root-level scratch logs. Use `build/` or another non-root
  scratch location.

## Working Model

The suspected immediate path is that `emit_prepared_module_text(...)` returns
only:

```asm
    .text
```

when `append_prepared_global_storage_asm(...)` rejects a global before function
assembly is emitted. Representative source cases include:

- `src/00024.c`: zero-initialized global struct storage with prepared BIR
  still containing `main`.
- `src/00025.c`: string literal and `strlen`, with prepared addressing using
  `kind=string_constant`.
- `src/00094.c`: unused `extern int x;` should not prevent `main` emission.

The final classification should distinguish at least:

- unused extern/global declarations that should be ignored by storage emission
- string literal storage or string-constant address materialization
- aggregate or struct global storage
- global aggregate load/store lowering
- floating scalar or global storage
- pointer-valued global data
- other unsupported module or function shapes

## Execution Rules

- Prefer generated exact case lists in `build/rv64_c_testsuite_probe_v2/` and
  concise durable summaries in follow-up idea text.
- Use prepared BIR evidence for non-trivial buckets; source string heuristics
  alone are not sufficient.
- When creating follow-up ideas, include concrete candidate cases, acceptance
  criteria, and reviewer reject signals.
- Keep changes limited to lifecycle, triage notes, generated scratch
  artifacts, and new focused idea files unless the supervisor explicitly
  delegates tiny instrumentation needed for classification.
- Treat testcase-overfit or expectation downgrades as route failure, not
  progress.

## Ordered Steps

### Step 1: Normalize the Probe Result

Goal: establish the current facts for the RV64 c-testsuite probe.

Primary target: `build/rv64_c_testsuite_probe_v2/`

Actions:

- Inspect existing probe artifacts if they are present.
- If artifacts are missing or stale, rerun a non-blocking probe that builds
  assembly, links with the RV64 clang toolchain, and runs qemu only as scratch
  investigation.
- Record the command, date, total cases, pass count, and failure buckets.
- Confirm whether the undefined-main bucket is still 93 cases, or document the
  changed count with evidence.

Completion check:

- The active notes or generated artifacts show reproducible total/pass/failure
  counts and identify the current undefined-main case list.

### Step 2: Capture Representative Evidence

Goal: prove the main failure mechanism for each rough source category.

Primary targets:

- representative c-testsuite sources such as `src/00024.c`, `src/00025.c`, and
  `src/00094.c`
- emitted RV64 asm
- prepared BIR dumps or summaries

Actions:

- For each representative category, capture the source shape, emitted asm
  head, prepared BIR global/function summary, and the reason RV64 storage or
  function emission skipped `main`.
- Confirm whether prepared BIR still contains `main` for representative
  undefined-main cases.
- Identify cases that are not caused by the suspected global-storage gate.

Completion check:

- Each non-trivial bucket has at least one representative prepared-BIR and asm
  explanation, and exceptions are called out separately.

### Step 3: Classify All Undefined-Main Cases

Goal: turn the noisy linker bucket into root-cause buckets.

Primary target: a generated or durable classification summary under a
non-root location such as `build/rv64_c_testsuite_probe_v2/`

Actions:

- Classify every undefined-main case into a root-cause bucket.
- Use prepared BIR or emitted asm evidence where source inspection is
  ambiguous.
- Separate fail-closed/output-contract defects from genuine missing RV64
  feature families.
- Preserve exact case lists for each bucket in an artifact future agents can
  reuse.

Completion check:

- All undefined-main cases are accounted for with bucket counts, exact case
  lists, and enough evidence to avoid repeating the sweep.

### Step 4: Decide Repair Ordering

Goal: choose follow-up repair slices that are focused and reviewable.

Actions:

- Put fail-closed contract cleanup before broad feature support when it blocks
  honest diagnostics.
- Order feature buckets by pass-count impact, implementation risk, and ability
  to validate without overfitting named cases.
- Avoid bundling unrelated feature families into one giant c-testsuite idea.

Completion check:

- The proposed repair order is documented with the reason each bucket should
  become its own follow-up or be grouped with a closely related bucket.

### Step 5: Create Focused Follow-Up Ideas

Goal: leave executable repair ideas under `ideas/open/`.

Primary target: `ideas/open/`

Actions:

- Create at least two focused follow-up idea files.
- Expected candidates include:
  - RV64 module/global storage fail-closed contract cleanup
  - unused extern global skip/ignore support
  - RV64 string literal storage and address-materialization foundation
  - RV64 aggregate/struct global storage foundation
  - RV64 floating scalar/global support, if bucket size justifies it
- Include concrete candidate cases, acceptance criteria, and reviewer reject
  signals in each follow-up idea.

Completion check:

- At least two focused ideas exist under `ideas/open/`, and future agents can
  start implementation without repeating this triage.

### Step 6: Close Triage Readiness Check

Goal: decide whether this source idea is complete.

Actions:

- Verify the classification summary covers the undefined-main bucket.
- Verify representative prepared-BIR/asm evidence exists for each
  non-trivial bucket.
- Verify follow-up ideas cover the chosen repair slices.
- Confirm no implementation feature support was snuck into the triage.

Completion check:

- The supervisor can ask the plan owner to close this triage idea, or identify
  the exact missing evidence or follow-up idea still needed.
