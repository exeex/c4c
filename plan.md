# Prepared Indirect Call String Argument Facts Runbook

Status: Active
Source Idea: ideas/open/310_prepared_indirect_call_string_argument_facts.md
Activated from: idea 309 Step 2 blocker

## Purpose

Restore the semantic producer facts needed by idea 309 before AArch64 indirect
call lowering resumes.

## Goal

Make indirect-call string pointer arguments retain their string-constant
identity in BIR/prepared data, matching the existing direct-call string
argument authority.

## Core Rule

Progress must come from BIR/prepared semantic fact publication. Do not make
AArch64 infer `.str*` arguments from assembly text, source shape, argument
position, `00189.c`, `stdout`, or `fprintfptr`.

## Read First

- `ideas/open/310_prepared_indirect_call_string_argument_facts.md`
- `ideas/open/309_aarch64_indirect_call_argument_preservation.md`,
  especially its deactivation note
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`, especially the
  direct-call string argument materialization coverage
- `src/backend/prealloc/stack_layout/coordinator.cpp`, especially direct
  string-constant call-argument materialization
- `tests/c/external/c-testsuite/src/00189.c`

## Current Scope

- BIR/prepared publication for string-constant pointer arguments passed to
  indirect calls.
- Prepared addressing/materialization records for the focused `00189.c`
  argument shape.
- Focused prepare or handoff tests that prove semantic string argument facts
  without using c-testsuite filename shortcuts.

## Non-Goals

- No AArch64 indirect callee register placement, call-register shuffle, `blr`
  preservation, or final assembly repair in this prerequisite idea.
- No direct multi-argument shuffle owner for `00181.c` or `00182.c`.
- No direct vararg aliasing/materialization owner for `00200.c`.
- No address-of-local direct-call argument preparation owner for `00218.c`.
- No generic string/pointer/store/control materialization bucket outside the
  indirect-call string argument fact needed here.
- No expectations, allowlists, unsupported classifications, runner policy,
  timeout policy, CTest registration, proof-log, or test-contract edits.

## Working Model

- Idea 309 cannot finish safely because the outer indirect call string
  argument reaches AArch64 as an ordinary register source with no string
  producer or prepared materialization.
- LIR still contains `%t2 = getelementptr ... @.str1`, so the semantic identity
  exists before the BIR/prepared handoff.
- Direct-call string arguments already publish prepared string-constant
  materializations. The next implementation route should make indirect-call
  arguments consume the same authority rather than adding a target-specific
  guess.
- After this prerequisite is complete, idea 309 should resume and repair the
  AArch64 callee/register preservation boundary, including the observed
  `fprintfptr` register-placement mismatch.

## Execution Rules

- Keep routine implementation progress in `todo.md`.
- Add tests at the prepare/handoff layer when they prove indirect-call string
  argument fact publication.
- Preserve direct-call string argument coverage while adding indirect-call
  parity.
- Stop and return to lifecycle if the repair requires a broader generic
  string/pointer materialization owner.

## Ordered Steps

### Step 1: Locate the Existing Direct-Call String Argument Authority

Goal: identify the direct-call producer path and the exact missing branch for
indirect calls.

Primary targets: `src/backend/prealloc/stack_layout/coordinator.cpp`,
`tests/backend/bir/backend_prepare_stack_layout_test.cpp`, and focused BIR /
prepared dumps for the `00189.c` shape.

Actions:

- Trace how direct-call LIR `getelementptr @.str*` arguments become prepared
  string-constant address materializations.
- Compare direct-call publication with the blocked indirect-call path where
  BIR keeps `%t2` only as a call argument reference.
- Identify the smallest shared producer or rewrite surface that can publish
  indirect-call string argument facts without target-specific inference.
- Do not edit AArch64 lowering, expectations, allowlists, runner policy, CTest
  registration, or proof logs.

Completion check:

- `todo.md` records the producer surface, the direct-call parity behavior, and
  the focused proof command delegated by the supervisor.

### Step 2: Publish Indirect-Call String Argument Facts

Goal: make indirect-call string pointer arguments publish semantic string
constant materialization facts before target lowering.

Actions:

- Implement the smallest semantic producer/handoff repair for indirect-call
  string arguments.
- Preserve string text identity, result value identity, byte offset, and
  address-space facts.
- Add or adjust focused prepare/handoff tests that would fail when indirect
  call arguments lack string-constant identity.
- Preserve existing direct-call string argument behavior.

Completion check:

- A fresh build or compile proof succeeds for the changed code.
- Focused prepared dumps or tests show the indirect-call string argument has a
  prepared string-constant materialization rather than only an ordinary
  register source.

### Step 3: Prove and Return to Idea 309

Goal: prove the prerequisite and hand lifecycle back to the AArch64 call-boundary
owner.

Actions:

- Run the supervisor-delegated focused prepare/handoff proof.
- If delegated, re-check `--dump-prepared-bir --mir-focus-function main` for
  `00189.c` and record whether `.str1` is now published semantically.
- Ask the plan owner to reactivate idea 309 if the missing producer fact is
  repaired.
- If a broader materialization gap remains, record the blocker in `todo.md`
  instead of claiming idea 309 is ready.

Completion check:

- Lifecycle state is ready to switch back to idea 309, or `todo.md` names the
  remaining BIR/prepared producer blocker and why it is outside this idea.
