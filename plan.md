# AArch64 Local Conversion Store/Load Publication Plan

Status: Active
Source Idea: ideas/open/347_aarch64_local_conversion_store_load_publication.md

## Purpose

Repair the residual AArch64 local conversion store/load publication failure
visible in `c_testsuite_aarch64_backend_src_00175_c` after direct-call
argument publication has advanced.

Goal: make local scalar and floating conversion results store to and reload
from the authoritative local home without weakening expectations or reopening
the completed direct-call owner.

Core Rule: fix the general local conversion publication rule; do not
special-case `00175`, named functions, individual registers, stack offsets, or
one emitted instruction sequence.

## Read First

- `ideas/open/347_aarch64_local_conversion_store_load_publication.md`
- Generated AArch64 output and runtime proof for
  `c_testsuite_aarch64_backend_src_00175_c`
- Existing direct-call representative guards from the prior owner:
  `00140`, `00159`, `00170`, and `00218`

## Current Targets

- Local conversion store/load publication in AArch64 generated code.
- The first residual `00175` mismatches after the direct-call series:
  `97 17`, `97 -581795216`, and `0.000000 0.000000`.
- Focused backend coverage for the local conversion store/load contract once
  the semantic rule is localized.

## Non-Goals

- Do not reopen direct-call argument or formal publication unless fresh
  first-bad-fact evidence shows a callsite ABI argument or callee formal home
  regression.
- Do not reopen closed indirect-call, selected call-boundary printer,
  return-result publication, callee-saved scalar-home, or scalar-select owners
  from filename recurrence alone.
- Do not change expectations, unsupported classifications, allowlists, CTest
  registration, timeout policy, proof-log policy, runner behavior, or root
  logs as part of this plan.
- Do not broaden into pointer/null condition publication, aggregate/table
  memory corruption, libc/file/string residuals, semantic `lir_to_bir`
  admission, timeout/output-storm, or unrelated backend cleanup.

## Working Model

- The direct-call argument publication path for the relevant `00175` calls has
  already advanced.
- The current residual is a later local scalar and floating conversion
  store/load publication failure.
- The authoritative value may be lost through conversion result materialization,
  local homes, stack slot selection, stores, or reloads; the first bad fact must
  decide which surface owns the repair.

## Execution Rules

- Localize before editing: record the first generated-code fact where the
  expected local conversion value is lost.
- Prefer a semantic lowering/publication repair over testcase-shaped matching.
- Keep coverage focused on the local conversion store/load contract where a
  unit-level backend test can pin it.
- Use `c_testsuite_aarch64_backend_src_00175_c` as representative proof after
  focused localization or coverage establishes the rule.
- Preserve direct-call guard stability, especially `00140`, `00159`, `00170`,
  and `00218`.

## Steps

### Step 1: Localize the Residual First Bad Fact

Goal: prove where the local conversion store/load path first loses the
authoritative scalar or floating value.

Primary target: generated AArch64 and runtime evidence for
`c_testsuite_aarch64_backend_src_00175_c`.

Actions:

- Inspect the post-direct-call region of `00175` and identify the first
  mismatching local scalar or FP conversion output.
- Trace the conversion value from materialization through local home selection,
  store, and reload.
- Distinguish the failing fact from direct-call ABI argument/formal
  publication with concrete generated-code evidence.
- Write the first-bad-fact summary into `todo.md` when the executor packet
  completes.

Completion check:

- `todo.md` names the first stale, uninitialized, or mismatched local
  conversion store/load fact and explains why it is not a direct-call ABI
  publication regression.

### Step 2: Repair the General Local Conversion Publication Rule

Goal: make local conversion results publish through the correct local home for
both scalar integer and floating conversions.

Primary target: the AArch64 backend path identified by Step 1.

Actions:

- Repair the semantic store/load or home-publication rule that owns the first
  bad fact.
- Keep the change general across local conversion results; do not key on
  `00175`, source line, function name, register number, stack offset, or a
  specific instruction text shape.
- Preserve existing direct-call publication behavior while changing only the
  local conversion path required by the first bad fact.

Completion check:

- The localized stale local conversion value is no longer stale for the same
  generated-code path, and the diff does not introduce testcase-shaped
  shortcuts or expectation changes.

### Step 3: Add Focused Backend Coverage

Goal: pin the local conversion store/load publication contract outside the
full c-testsuite representative when a focused test surface is available.

Primary target: the narrow backend test bucket that already covers AArch64
publication or local homes.

Actions:

- Add or extend focused backend coverage for the repaired local conversion
  store/load behavior.
- Cover both the scalar integer and floating conversion sides when the localized
  rule applies to both.
- Keep assertions tied to semantic publication behavior rather than incidental
  register, offset, or instruction-sequence details.

Completion check:

- The focused backend test fails before the semantic repair or directly proves
  the repaired contract, and passes after the repair without expectation
  weakening.

### Step 4: Prove Representative Progress

Goal: prove `00175` advances or passes for the local conversion residual.

Primary target: `c_testsuite_aarch64_backend_src_00175_c`.

Actions:

- Run the supervisor-delegated proof command for `00175`.
- Confirm the local scalar and floating conversion outputs advance or pass.
- If `00175` still fails, reclassify the remaining failure by its next first
  bad fact before claiming this idea complete.

Completion check:

- `00175` no longer shows the targeted local conversion store/load mismatch,
  or `todo.md` documents the next distinct first bad fact that remains.

### Step 5: Check Direct-Call Guard Stability

Goal: ensure this local conversion repair did not regress the prior direct-call
publication owner.

Primary target: direct-call representative guards `00140`, `00159`, `00170`,
and `00218`.

Actions:

- Run the supervisor-delegated guard subset for the direct-call
  representatives.
- Investigate any regression as route drift unless generated-code evidence
  ties it to the local conversion publication repair.

Completion check:

- The direct-call representative guards remain stable, and proof results are
  recorded in `todo.md`.
