# AArch64 Return Result Publication Epilogue Clobber Runbook

Status: Active
Source Idea: ideas/open/336_aarch64_return_result_publication_epilogue_clobber.md

## Purpose

Focus the umbrella backend-regex inventory on the AArch64 runtime family where
generated functions compute or load the intended result into `w0`/`x0`, then
clobber `x0` with a stale restored register before returning.

## Goal

Repair the general AArch64 return-result publication / epilogue-clobber owner
for scalar, pointer/local, and call-result representatives without changing
test expectations or runner behavior.

## Core Rule

Do not fix this by matching c-testsuite filenames, literal registers, or final
instruction strings. Localize and repair the semantic rule that decides the
authoritative return result and prevents epilogue or restore code from
overwriting it.

## Read First

- `ideas/open/336_aarch64_return_result_publication_epilogue_clobber.md`
- current `todo.md`
- current `test_after.log` classification evidence from umbrella idea 295
- generated assembly under `build/c_testsuite_aarch64_backend/src/` for:
  `00004.c.s`, `00011.c.s`, `00022.c.s`, `00052.c.s`, `00087.c.s`,
  `00124.c.s`, `00159.c.s`, and `00168.c.s`
- closed ideas 333 and 335 only to preserve their boundaries, not to reopen
  them from counts alone

## Current Targets

- Return-result clobber representatives from the current 22-case bucket:
  `00004`, `00011`, `00013`, `00014`, `00016`, `00019`, `00020`, `00022`,
  `00052`, `00087`, `00103`, `00112`, `00116`, `00117`, `00118`, `00121`,
  `00124`, `00139`, `00153`, `00159`, `00168`, and `00170`.
- No-call scalar return cases that materialize a result and then emit stale
  `mov x0, x13` or equivalent.
- Pointer/local return cases that load the correct `w0` value and then emit
  stale `mov x0, x20` or equivalent.
- Call-result cases that compute or preserve a call result and then return a
  restored/stale callee-saved register.

## Non-Goals

- Do not reopen closed ideas 333 or 335 without generated-code evidence tying
  the current first bad fact to their exact old owners.
- Do not edit expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, or CTest registration.
- Do not repair scalar conversion/comparison, addressable-memory
  materialization, switch/loop/control-flow lowering, varargs/libc ABI,
  indirect-call pointer materialization, scalar-cast machine-printer, or
  timeout/output-storm buckets from idea 295 under this plan.
- Do not broad-rewrite AArch64 prologue/epilogue, register allocation, or call
  lowering unless localization proves that narrower owner is insufficient.

## Working Model

The current failure signature is not that `w0`/`x0` can never receive the
right value. Multiple representatives show the right result reaching `w0` or
`x0` before a later return-path move overwrites it with a stale saved register.
The repair should preserve the ABI return register as the authoritative return
value after expression/call result publication and through epilogue emission.

## Execution Rules

- Start with generated assembly and prepared/backend dumps before changing
  code.
- Prove at least three representative shapes before claiming the owner:
  no-call scalar return, pointer/local return, and call-result return.
- Add focused backend coverage for the repaired rule where practical before
  relying on external c-testsuite proof.
- Treat a new first bad fact after removing the stale return overwrite as a
  residual for classification, not automatic scope expansion.
- Keep proof logs in the canonical files selected by the supervisor.

## Ordered Steps

### Step 1: Localize Return Result Clobber Sites

Goal: identify where each representative's intended return value is produced,
published to `w0`/`x0`, and then overwritten.

Primary targets: generated assembly and focused dumps for one no-call scalar
case, one pointer/local case, and one call-result case.

Actions:

- Inspect `00011`, `00022`, or `00052` for the simple scalar return shape.
- Inspect `00004` for the pointer/local return shape.
- Inspect `00159`, `00168`, `00087`, or `00124` for call-result preservation.
- Record the first point where the intended return value loses authority to a
  stale restored register.
- Decide whether the owner is return home selection, result publication,
  call-result preservation, or epilogue/restore ordering.

Completion check:

- `todo.md` records the concrete first bad fact and the narrow repair target
  with representative evidence from at least two shapes.

### Step 2: Repair The Narrow Return Publication Owner

Goal: prevent return-path code from overwriting the authoritative `w0`/`x0`
result with a stale register.

Primary target: the localized backend component from Step 1.

Actions:

- Apply the smallest semantic repair that preserves the ABI return register
  through function return emission.
- Keep the change independent of c-testsuite filenames and literal register
  numbers.
- Add or update focused backend coverage that fails on the old stale-register
  clobber and passes with the repair.
- Rebuild or compile the touched target as required by the supervisor packet.

Completion check:

- Focused backend coverage passes and generated representative assembly no
  longer shows a correct result followed by a stale final return overwrite.

### Step 3: Prove Representative Runtime Shapes

Goal: verify the repair across the return-result bucket's distinct shapes.

Primary target: supervisor-selected focused c-testsuite subset.

Actions:

- Run one no-call scalar return representative such as `00011`, `00022`, or
  `00052`.
- Run `00004` or another pointer/local representative.
- Run one call-result representative such as `00159`, `00168`, `00087`, or
  `00124`.
- If a representative advances to a new first bad fact, record the residual
  without expanding this plan unless it is the same return-result owner.

Completion check:

- The selected representatives either pass or advance past the stale return
  overwrite with any residual clearly classified.

### Step 4: Broader Guardrail And Handoff

Goal: prove the focused repair did not regress adjacent backend behavior and
decide whether this source idea can close.

Primary target: supervisor-selected backend subset and canonical proof logs.

Actions:

- Run the supervisor-selected backend guardrail after focused proof is green.
- Compare against closed idea 333 and 335 boundaries to avoid accidental
  reopening or regression.
- Update `todo.md` with proof results and any residual owner candidate.
- Return to plan-owner for closure or lifecycle handoff when the source idea's
  acceptance criteria are satisfied or a separate residual owner is exposed.

Completion check:

- Focused and guardrail proof is recorded, no expectation/runner changes were
  used, and closure or next-owner handoff is ready for plan-owner review.
