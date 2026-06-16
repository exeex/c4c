# Retire Rendered Call-Argument ABI Suffix Fallback

Status: Active
Source Idea: ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md

## Purpose

Finish the post-286 call-argument interface cleanup by removing, quarantining,
or strictly fencing the remaining rendered `alignstack(...)` ABI suffix fallback
from semantic BIR call lowering.

## Goal

Supported structured call-argument lowering must take type/layout authority from
structured metadata, not from rendered call-argument text.

## Core Rule

Do not make progress by renaming or relocating
`strip_call_arg_abi_type_suffix` while preserving the same structured semantic
dependency on rendered ABI suffix parsing.

## Read First

- `ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md`
- Current callers of `strip_call_arg_abi_type_suffix`
- Existing 286/288 AArch64 variadic HFA carrier tests:
  - `backend_lir_to_bir_notes`
  - `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`

## Current Targets

- `strip_call_arg_abi_type_suffix` call sites in semantic BIR call lowering.
- The structured call-argument metadata boundary around `LirCallArg`,
  `LirTypeRef`, or any dedicated call-argument ABI metadata field.
- Raw/no-ref rendered call-argument compatibility behavior, if it still needs
  to exist.
- Focused tests that prove structured metadata wins over stale rendered suffix
  text and that any legacy rendered fallback is fail-closed.

## Non-Goals

- Do not rewrite AArch64 ABI classification or prepared call-plan construction.
- Do not reopen the AAPCS64 HFA lane helper boundary from idea 288.
- Do not migrate public prepared call-plan surfaces or delete prepared fallback
  behavior.
- Do not use hand-built test LIR convenience as evidence that production
  semantic lowering still needs rendered suffix parsing.
- Do not broaden this into unrelated rendered-text cleanup.

## Working Model

- Structured call-argument paths should carry the ABI/layout facts needed by
  semantic lowering.
- Rendered `alignstack(...)` text may remain only if it is behind an explicit
  raw/no-ref legacy predicate and cannot rescue a structured metadata mismatch.
- Test fixture gaps should be fixed as fixture or builder gaps, not by keeping
  production structured lowering dependent on rendered text.

## Execution Rules

- Preserve the completed 286/288 AArch64 variadic HFA carrier behavior.
- Prefer small steps that separate inventory, ownership decision, production
  routing, and proof.
- For code-changing steps, run a build or compile proof plus the narrow tests
  named in the step.
- If a call site is separate carrier work, record the classification in
  `todo.md` and do not silently expand this plan.
- Do not weaken supported-path tests, mark supported cases unsupported, or
  rewrite expectations as the main evidence of progress.

## Steps

### Step 1: Inventory and classify suffix parser use

Goal: establish every current use of `strip_call_arg_abi_type_suffix` and the
correct owner category for each.

Primary target: all current `strip_call_arg_abi_type_suffix` declarations,
definitions, and call sites.

Actions:

- Search the tree for `strip_call_arg_abi_type_suffix`.
- For each production call site, classify it as one of:
  - structured call-argument path that should not parse rendered suffixes;
  - raw/no-ref legacy compatibility path;
  - test-only fixture construction gap;
  - separate carrier work.
- Record the inventory and classifications in `todo.md`.
- Identify which structured metadata surface should own `alignstack` for the
  supported aggregate call paths.

Completion check:

- `todo.md` contains a complete call-site inventory with final or provisional
  classifications.
- The next implementation step has a named metadata owner candidate and a
  scoped target list.

### Step 2: Route structured lowering through metadata

Goal: remove rendered suffix parsing from structured semantic call lowering
where structured metadata is present.

Primary target: semantic BIR call lowering around the structured call-argument
path.

Actions:

- Thread or read `alignstack` authority from the chosen structured owner.
- Stop using rendered `alignstack(...)` text to decide structured aggregate
  type/layout facts.
- Fix hand-built test helpers or fixtures that were relying on rendered suffix
  text to stand in for structured metadata.
- Keep any raw/no-ref behavior outside the structured route.

Completion check:

- Structured call-argument lowering has a clear metadata-based path.
- No structured supported path depends on
  `strip_call_arg_abi_type_suffix` for semantic authority.
- Build or compile proof is fresh for the touched target.

### Step 3: Quarantine or remove legacy rendered fallback

Goal: either remove the rendered suffix fallback or fence it as explicit
legacy/raw-no-ref compatibility.

Primary target: remaining `strip_call_arg_abi_type_suffix` use after Step 2.

Actions:

- Delete the fallback if no supported path still requires it.
- If compatibility remains necessary, guard it behind an explicit legacy/raw
  no-ref predicate.
- Ensure structured metadata mismatch cannot be rescued by rendered suffix
  text.
- Keep the compatibility predicate narrow and auditable.

Completion check:

- Any remaining rendered suffix parser call is justified as legacy-only in
  `todo.md`.
- A structured mismatch fails closed instead of falling back to rendered text.
- No new named-case or testcase-shaped shortcut was introduced.

### Step 4: Add focused proof coverage

Goal: prove the interface boundary and preserve the completed 286/288 behavior.

Primary target: backend LIR-to-BIR notes and CLI dump tests covering the
affected call-argument lowering route.

Actions:

- Add focused tests showing structured metadata wins over stale or mismatched
  rendered `alignstack(...)` suffix text.
- Add focused tests showing any remaining raw/no-ref legacy fallback is
  quarantined and cannot broaden supported semantic lowering.
- Run the existing 286/288 proof subset:
  - `backend_lir_to_bir_notes`
  - `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`

Completion check:

- Focused proof covers structured precedence and any legacy fallback fence.
- The existing 286/288 proof subset is green.
- `todo.md` records the exact proof commands and results.

### Step 5: Closure inventory and handoff

Goal: prepare the source idea for closure review without changing source intent
prematurely.

Primary target: `todo.md` summary and final lifecycle close request.

Actions:

- Summarize the final `strip_call_arg_abi_type_suffix` inventory and
  classifications.
- State whether the parser was removed or retained only behind a legacy fence.
- Capture any separate carrier work as a follow-up idea instead of expanding
  this plan.
- Ask the supervisor to run close-time regression guard through the plan-owner
  workflow.

Completion check:

- The acceptance criteria from the source idea are demonstrably met or any
  remaining blocker is explicit.
- Closure notes include inventory, proof commands, and residual follow-up
  boundaries.
