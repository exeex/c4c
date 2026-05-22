# AArch64 Scalar Cast Stack-Homed Register Source Publication Refresh Runbook

Status: Active
Source Idea: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md

## Purpose

Refresh the parked scalar-cast stack-homed register source-publication owner and
decide whether the source idea can close or must remain parked.

## Goal

Confirm whether any current AArch64 scalar cast still reaches selected machine
printing without a structured register source after a prepared
consumer stack-to-register move exists.

## Core Rule

Do not repair, classify, or close this idea by changing expectations,
unsupported status, runner behavior, timeout policy, CTest registration, or
proof-log policy. Any in-scope work must address the scalar cast register-source
publication boundary generally, not `00143` by name.

## Read First

- `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
- Existing scalar cast and prepared scalar cast backend coverage.
- Generated/prepared evidence for `c_testsuite_aarch64_backend_src_00143_c`
  only as representative proof after focused scalar-cast coverage is checked.

## Current Scope

- Refresh the focused scalar-cast proof for stack-homed sources that already
  have prepared consumer stack-to-register moves.
- Check whether `c_testsuite_aarch64_backend_src_00143_c` still fails with the
  old structured-register-source diagnostic.
- If the old first bad fact is gone, preserve that as closure evidence and
  hand lifecycle control back to the supervisor for the close gate.
- If a new first bad fact appears, classify whether it remains inside this
  source idea or requires a separate lifecycle handoff.

## Non-Goals

- Do not reopen local scalar storage/writeback sizing, runtime value
  correctness, frame layout, compare lowering, aggregates, variadics, or
  semantic admission unless fresh generated-code evidence proves the scalar
  cast first bad fact moved there.
- Do not edit implementation files during lifecycle activation.
- Do not broaden the representative beyond scalar cast source publication
  without a lifecycle split.

## Working Model

The source idea is parked because prior execution repaired the known missing
structured register source for selected scalar casts with stack-homed inputs.
The current lifecycle question is not to invent more scalar-cast work; it is to
refresh the current tree and determine whether a live in-scope first bad fact
still exists.

## Execution Rules

- Use `todo.md` for packet progress and proof notes.
- Keep `plan.md` stable unless the refresh reveals a real route correction.
- Keep source-idea edits out of routine execution. The source idea should only
  change if closure, deactivation notes, or a separate durable initiative are
  required.
- Treat a matching green focused subset as classification evidence, not as an
  automatic close. Close still requires the lifecycle close gate.

## Ordered Steps

### Step 1: Refresh Current Scalar-Cast Proof

Goal: Determine whether the old structured-register-source scalar cast failure
is still present.

Concrete actions:

- Run a focused build plus scalar-cast proof selected by the supervisor.
- Include the focused backend scalar cast record tests and
  `c_testsuite_aarch64_backend_src_00143_c`.
- Inspect failures, if any, for the old diagnostic:
  `scalar cast node requires a structured register source operand`.
- If the subset is green, record that no current in-scope first bad fact is
  visible.
- If the subset is red, classify the first bad fact against the source idea's
  in-scope and out-of-scope boundaries.

Completion check:

- `todo.md` records the exact command, result, and whether a live in-scope
  scalar-cast source-publication first bad fact exists.

### Step 2: Closure Or Park Decision

Goal: Decide whether this active runbook can be closed, deactivated, or handed
off to a separate source idea.

Concrete actions:

- If Step 1 finds no live in-scope first bad fact, ask the plan owner close
  flow to apply the required regression guard before archival closure.
- If the close gate rejects only because the strict pass-count rule cannot be
  satisfied, deactivate this runbook and leave the source idea parked.
- If Step 1 finds an out-of-scope first bad fact, preserve only the durable
  handoff note needed for the correct source idea.
- If Step 1 finds an in-scope first bad fact, keep this plan active for a
  bounded executor repair packet.

Completion check:

- Exactly one of these is true: the idea is closed under the close gate; the
  runbook is deactivated with the source idea still parked; or `todo.md`
  points to a bounded repair packet for the refreshed in-scope first bad fact.
