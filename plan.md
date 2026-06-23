# Variadic Target IR ABI Boundary Triage Runbook

Status: Active
Source Idea: ideas/open/325_variadic_target_ir_abi_boundary_triage.md

## Purpose

Map where variadic calls and `stdarg` lowering must diverge between AArch64 and
RV64 before any RV64 variadic runtime-support work starts.

Goal: produce target ABI evidence, identify the first c4c layer that must carry
target-specific variadic facts, and create focused follow-up ideas for the next
implementation boundaries.

Core Rule: keep RV64 variadic support fail-closed during this triage; do not
replace unsupported diagnostics with fake calls, stubs, expectation downgrades,
or named-testcase shortcuts.

## Read First

- Source idea: `ideas/open/325_variadic_target_ir_abi_boundary_triage.md`
- Debug flag reference: `.codex/skills/c4cll-debug-flags/SKILL.md`
- Relevant AArch64 history:
  - `ideas/closed/73_aarch64_variadic_prepared_entry_plan_cleanup.md`
  - `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md`
  - `ideas/closed/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md`
  - `ideas/closed/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md`
  - `ideas/closed/292_reopen_286_288_match_load_local_memory_admission.md`

## Current Scope

- Direct external variadic calls such as `printf`.
- Variadic callee entry setup for `va_start`.
- `va_list` layout and storage facts.
- Scalar `va_arg` extraction, with optional aggregate or floating evidence only
  if it clarifies ABI boundaries.
- c4c LLVM-route output, semantic BIR, and prepared BIR for the same tiny
  fixtures and target triples.

## Non-Goals

- Do not implement RV64 `printf`, variadic external calls, `va_start`, or
  `va_arg` runtime support in this triage.
- Do not weaken the current RV64 fail-closed diagnostics for unsupported
  variadic external calls.
- Do not add fake libc stubs, fake `printf` bodies, or c-testsuite-specific
  symbol shortcuts.
- Do not copy AArch64 variadic constants into RV64 without ABI evidence.
- Do not register the full variadic c-testsuite bucket as required coverage for
  this triage.

## Working Model

- Clang target LLVM IR is the external evidence for where ABI divergence begins.
- c4c has no standalone `--dump-lir` flag in the current CLI; use
  `--codegen llvm` for LLVM-route IR evidence.
- Use `--dump-bir` for semantic backend IR and `--dump-prepared-bir` for
  backend-facing prepared ABI and ownership facts.
- Compare the same fixture across `aarch64-linux-gnu` and
  `riscv64-linux-gnu`; do not infer RV64 behavior from AArch64 alone.
- Keep generated `.c`, `.ll`, `.bir.txt`, `.prepared.txt`, and notes under a
  scratch directory such as `build/variadic_abi_triage/`.

## Execution Rules

- Record exact commands and artifact paths used for every evidence point.
- If a c4c command is unsupported or fails before the intended dump stage,
  record the failure as boundary evidence instead of patching around it.
- Separate evidence for external variadic calls from true `stdarg` variadic
  callee lowering.
- Treat `--codegen llvm` as LLVM-route comparison evidence, not proof that the
  backend route is healthy.
- Follow-up ideas under `ideas/open/` must each name one implementation
  boundary and proof strategy; do not create a single broad "support variadic"
  idea.

## Steps

### Step 1: Prepare Minimal Evidence Fixtures

Goal: create the smallest reproducible input set for target ABI comparison.

Primary target: `build/variadic_abi_triage/`

Actions:

- Create a direct external variadic call fixture:
  `int printf(const char *, ...); int main(void) { return printf("%d\n", 7); }`
- Create a scalar `stdarg` fixture using `va_start`, one `va_arg(ap, int)`,
  `va_end`, and a caller that checks the result.
- Optionally create one aggregate or floating `va_arg` fixture only if scalar
  evidence leaves an ABI-boundary question unanswered.
- Record fixture filenames and the exact target triples used.

Completion check:

- The scratch directory contains the selected fixtures and a short notes file
  explaining why each fixture is included.

### Step 2: Capture Clang Target LLVM IR Evidence

Goal: establish the external AArch64 versus RV64 ABI differences before reading
c4c output.

Primary target: clang-generated LLVM IR artifacts in
`build/variadic_abi_triage/`.

Actions:

- Run clang for every selected fixture:
  - `clang --target=aarch64-linux-gnu -S -emit-llvm <case.c> -o <case.aarch64.ll>`
  - `clang --target=riscv64-linux-gnu -S -emit-llvm <case.c> -o <case.rv64.ll>`
- Compare function signatures, datalayout, variadic call operands, `va_list`
  representation, intrinsics, loads, stores, alignments, and target-specific
  helper calls.
- Write concise notes that separate direct external call ABI facts from
  `va_start`/`va_arg` callee facts.

Completion check:

- Notes identify concrete clang IR differences for at least one direct
  variadic call and one scalar `va_start`/`va_arg` case.

### Step 3: Capture c4c LLVM-Route, Semantic BIR, and Prepared BIR Evidence

Goal: find where c4c currently preserves, flattens, or lacks target-specific
variadic ABI facts.

Primary targets: `./build/c4cll` dumps for the same fixtures and target triples.

Actions:

- For every selected fixture and target, capture LLVM-route output:
  - `./build/c4cll --codegen llvm --target TRIPLE CASE.c -o CASE.c4c.TARGET.ll`
- For every selected fixture and target, capture semantic BIR:
  - `./build/c4cll --dump-bir --target TRIPLE CASE.c > CASE.c4c.TARGET.bir.txt`
- For every selected fixture and target, capture prepared BIR:
  - `./build/c4cll --dump-prepared-bir --target TRIPLE CASE.c > CASE.c4c.TARGET.prepared.txt`
- If a dump fails, preserve stderr/stdout as an artifact and record the failure
  stage and diagnostic.
- Compare c4c LLVM-route output to clang IR before interpreting BIR.
- Compare semantic BIR and prepared BIR for explicit call, callee-entry,
  `va_list`, storage, and `va_arg` facts.

Completion check:

- Notes identify the first c4c layer where target-specific variadic ABI facts
  are missing, flattened, or still present.

### Step 4: Inventory Existing AArch64 Variadic Mechanisms

Goal: determine which existing AArch64 records are reusable concepts and which
are target-specific details.

Primary targets: AArch64 variadic closed ideas and relevant code/tests found
from those ideas.

Actions:

- Read the closed AArch64 ideas listed in "Read First".
- Trace the named helpers, records, tests, and prepared-BIR sections they
  reference.
- Classify each mechanism as one of:
  - shared semantic/BIR concept;
  - AArch64 ABI-specific record or constant;
  - prepared ABI planning surface;
  - final target emitter behavior.
- Do not change implementation files during this inventory.

Completion check:

- Notes explain what RV64 can consume analogously versus what must be newly
  modeled from RV64 ABI evidence.

### Step 5: Decide the Next Repair Boundary

Goal: convert the evidence into a narrow implementation-boundary decision.

Primary target: triage notes under `build/variadic_abi_triage/`.

Actions:

- Decide whether the first repair belongs in:
  - target-aware LLVM-route IR production;
  - semantic LIR-to-BIR/BIR facts;
  - shared prealloc or prepared ABI records;
  - RV64 target emitter consumption.
- Explain how the boundary differs for:
  - external variadic call ABI;
  - variadic callee entry setup;
  - `va_list` layout;
  - `va_arg` extraction.
- Identify proof commands or fixture-level tests that would prove the next
  implementation slice without broad c-testsuite registration.

Completion check:

- The boundary decision is written clearly enough for a follow-up idea to own
  one implementation slice without redoing the whole triage.

### Step 6: Create Focused Follow-Up Ideas

Goal: leave durable next-step source ideas without expanding this triage into
implementation work.

Primary target: new files under `ideas/open/`.

Actions:

- Create at least one follow-up idea under `ideas/open/` with a concrete
  implementation boundary and proof strategy.
- Prefer one target-dependent IR/LIR or BIR-fact idea if clang comparison proves
  divergence starts before prepared BIR.
- Create an RV64-specific implementation idea only after required target ABI
  records are known.
- Include concrete reviewer reject signals in every follow-up idea.

Completion check:

- At least one new open idea exists, and no follow-up idea claims broad
  variadic support without separating IR boundary, ABI records, and target
  emission.

## Final Acceptance

- Clang AArch64 and RV64 LLVM IR evidence exists for at least one direct
  variadic call and one scalar `va_start`/`va_arg` case.
- c4c LLVM-route, semantic BIR, and prepared-BIR evidence exists for the same
  cases, or unavailable/failing stages are explicitly documented.
- The triage identifies the first c4c layer where target-specific variadic ABI
  facts must exist.
- The writeup separates external variadic call ABI, variadic callee entry setup,
  `va_list` layout, and `va_arg` extraction.
- At least one focused follow-up idea is created under `ideas/open/`.
- RV64 unsupported variadic diagnostics remain fail-closed unless a later
  implementation idea proves real support.
