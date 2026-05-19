# AArch64 Large Scalar Immediate Materialization Runbook

Status: Active
Source Idea: ideas/open/307_aarch64_large_scalar_immediate_materialization.md
Activated from: focused split out of umbrella inventory idea 295

## Purpose

Repair the focused post-306 assembler-stage residual where AArch64 scalar
constant materialization emits an illegal large immediate move.

## Goal

Make large scalar integer constants lower to legal AArch64 materialization
sequences instead of reaching assembly as unencodable `mov xN, #imm` forms.

## Core Rule

Progress must be semantic constant-materialization behavior. Do not special
case `00182.c`, the literal `1234567`, or one emitted instruction string, and
do not improve counts through expectations, allowlists, unsupported
classifications, runner policy, timeout policy, CTest registration, or proof
log changes.

## Read First

- `ideas/open/307_aarch64_large_scalar_immediate_materialization.md`
- `todo.md`
- `test_before.log`
- Generated artifacts for `c_testsuite_aarch64_backend_src_00182_c` under
  `build/c_testsuite_aarch64_backend/`
- Closed owner 299 for scalar instruction-immediate fallback and closed owner
  306 for symbol+offset address-register-width legality as boundary context

## Current Scope

- AArch64 scalar integer constant materialization for large immediates.
- Focused failure `c_testsuite_aarch64_backend_src_00182_c`.
- Nearby scalar immediate materialization probes only when selected by the
  supervisor to prove the rule is not testcase-shaped.

## Non-Goals

- No PIC/global-symbol relocation work for `00189.c`.
- No runtime nonzero, mismatch, crash, timeout, or output-storm repair.
- No call-boundary move, scalar `mul` printable-rhs, unprepared frame-slot
  source, or semantic `lir_to_bir` admission work.
- No reopening closed owners 285 through 306 from residual counts alone.
- No expectation, allowlist, unsupported, timeout, runner, CTest registration,
  or proof-log edits.

## Working Model

- The accepted post-306 backend-regex baseline is `test_before.log`: 352
  selected, 306 passed, 46 failed, all `c_testsuite_aarch64_backend_*`.
- `00182.c` now fails at assembler stage on `mov x0, #1234567` after the old
  symbol+offset address-register-width failure was closed by idea 306.
- `00189.c` is a different assembler/linker residual: non-PIC/global-symbol
  relocation against `stdout`.
- Runtime buckets remain parked under umbrella idea 295 until generated-code
  evidence justifies separate owners.

## Execution Rules

- Keep routine packet progress and proof notes in `todo.md`.
- Inspect generated assembly and backend lowering enough to identify where the
  illegal scalar constant reaches printing.
- Prefer existing backend constant materialization or legal immediate helpers
  over adding testcase-shaped branches.
- After any implementation slice, run the supervisor-delegated build and
  focused proof exactly; use `test_after.log` only as the delegated proof log.
- Report residual focused failures by their actual new failure mode instead of
  treating a pass-count change as closure by itself.

## Ordered Steps

### Step 1: Localize The Illegal Immediate Path

Goal: identify the backend path that emits the illegal large scalar immediate.

Primary target: generated artifacts for `c_testsuite_aarch64_backend_src_00182_c`

Actions:

- Inspect the generated AArch64 assembly and diagnostics for `00182.c`.
- Trace the scalar constant through AArch64 lowering/selection/printing to the
  point where `mov x0, #1234567` is formed.
- Check closed-owner boundaries for idea 299 and idea 306 before deciding
  whether to reuse an existing helper.

Completion check:

- `todo.md` records the responsible backend path, the old illegal emitted
  form, and the proposed semantic repair point.

### Step 2: Implement Legal Large-Immediate Materialization

Goal: lower large scalar constants through legal AArch64 forms before printing.

Actions:

- Implement the narrow backend constant-materialization repair at the semantic
  owner point identified in Step 1.
- Preserve existing behavior for encodable immediates and unrelated symbol,
  memory, call-boundary, runtime, and `lir_to_bir` paths.
- Avoid filename, literal-value, or exact-instruction-string matching.

Completion check:

- The focused generated assembly for `00182.c` no longer contains the old
  illegal large-immediate move, and the implementation route is not
  testcase-shaped.

### Step 3: Prove The Focused Owner

Goal: validate the repair without folding unrelated parked buckets into this
owner.

Actions:

- Run the supervisor-delegated build proof.
- Run the supervisor-delegated focused backend c-testsuite proof for `00182.c`
  and any selected nearby scalar immediate materialization probes.
- If broader backend-regex proof is delegated, keep `00189.c` PIC/global
  relocation and runtime buckets reported separately.

Completion check:

- `todo.md` records fresh proof, the old assembler failure is gone, and any
  remaining focused residual is classified outside or inside this owner by
  generated-code evidence.

### Step 4: Close Or Return To Umbrella

Goal: decide whether the source idea is complete after proof.

Actions:

- If acceptance criteria are satisfied, report the owner ready for lifecycle
  close.
- If the old failure is gone but a separate residual remains, record the
  boundary clearly so the supervisor can return to umbrella inventory.
- If the repair exposes a distinct initiative, ask the supervisor to split
  through lifecycle state instead of expanding this owner.

Completion check:

- The active lifecycle state has enough proof and boundary notes for the
  supervisor to request close, additional execution, or a return to umbrella
  inventory.
