# RV64 Instruction Fragment Consumers Runbook

Status: Active
Source Idea: ideas/open/612_rv64_instruction_fragment_consumers.md

## Purpose

Activate idea 612 as the current execution route for RV64/MIR instruction
fragment consumer lowering after move-bundle target materialization and
terminator lowering have run first.

## Goal

Repair supported ordinary-C RV64/MIR instruction-fragment consumers across
clearly sub-bucketed fragment families while preserving accurate rejection for
producer, prepared-authority, floating, vector, and policy-owned rows.

## Core Rule

Implement semantic RV64 consumer rules only for a proven instruction-fragment
family with complete upstream facts; do not infer missing BIR semantics or
prepared authority from final object shape, and do not merge unrelated fragment
families into one patch.

## Read First

- `ideas/open/612_rv64_instruction_fragment_consumers.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`

## Current Targets

- RV64 backend-object rows whose first reproduced stop is
  `unsupported_instruction_fragment`.
- Binary/pointer op rows, cast rows, and other instruction-fragment rows after
  prepared handoff.
- Negative rows where the first owner is still BIR semantic production,
  prepared authority, select publication, floating-only policy, vector policy,
  inline asm policy, ABI, global data, or runtime behavior.

## Non-Goals

- Do not implement BIR semantic cast, binop, pointer, or local/global memory
  production.
- Do not change move-bundle target materialization, terminator lowering, ABI
  lowering, runtime behavior, expectations, unsupported markers, allowlists,
  timeout/accounting, or GCC torture classification metadata.
- Do not include floating-only, vector, inline asm, select-publication, or
  library-policy lanes unless refreshed evidence proves they are required
  ordinary-C instruction-fragment consumers under this idea.
- Do not add filename-, row-, or testcase-shaped shortcuts.

## Working Model

- Treat the current `42` binary/pointer, `23` cast, and `32` other
  instruction-fragment counts as stale planning breadth until refreshed
  diagnostics confirm the live residual population.
- Sub-bucket before implementation when rows are mixed by operation family,
  value type, operand source, width, or upstream authority.
- Prefer one general family at a time: for example, an integer binary/pointer
  operation group, a cast group, or a narrowly defined other-fragment group.
- Keep rows fail-closed when the required producer fact, operand materialized
  value, width, address/pointer authority, or prepared fragment shape is absent
  or ambiguous.

## Execution Rules

- Start Step 1 from current diagnostics rather than stale bucket counts.
- Record the chosen implementation family and proof command in `todo.md`
  before code-changing Step 2 work begins.
- Each code-changing step must run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Use focused direct object probes for representative rows when backend logs
  are stale or ambiguous.
- Escalate to reviewer if a diff weakens unsupported contracts, rewrites
  expectations, moves producer repair into RV64, or proves progress only by
  matching a named source file.

## Step 1: Refresh And Sub-Bucket Instruction-Fragment Diagnostics

Goal: identify the current unsupported instruction-fragment population and
select one clean first implementation family.

Primary target:
- RV64 backend-object logs and focused probes for rows reporting
  `unsupported_instruction_fragment`.

Actions:
- Rebuild or inspect current backend logs for the RV64 gcc_torture object
  route.
- Collect current representative rows for binary/pointer, cast, select,
  floating cast, and other instruction-fragment diagnostics.
- Split rows by first owner:
  - RV64 consumer candidate with complete prepared fragment facts
  - missing BIR semantic producer fact
  - missing prepared authority or operand materialization
  - select publication or destination/source wiring
  - floating-only, vector, inline asm, ABI, global-data, runtime, or policy
    owner
- Choose one implementation family with more than one nearby row and a clear
  shared lowering rule.
- Record the exact Step 2 packet, positive rows, negative guard rows, and proof
  command in `todo.md`.

Completion check:
- `todo.md` names the refreshed row groups, the selected Step 2 family, at
  least two positive candidates when available, relevant negative guard rows,
  and the exact proof command.

## Step 2: Implement First Supported Instruction-Fragment Family

Goal: lower one general, explicitly authorized instruction-fragment family in
the RV64/MIR consumer.

Primary target:
- RV64/MIR object-emission instruction lowering path selected by Step 1.

Actions:
- Locate the RV64 consumer rejection for the selected fragment family.
- Add the smallest semantic lowering rule shared by the selected rows.
- Require complete upstream fragment facts, operand sources, value widths, and
  any pointer/address authority needed by that family.
- Preserve existing rejection for missing producer facts, ambiguous prepared
  authority, unsupported widths, and out-of-scope fragment classes.
- Avoid expectation, unsupported-marker, allowlist, timeout, runtime, and
  accounting changes.

Completion check:
- Multiple rows from the selected family compile or move past
  `unsupported_instruction_fragment`.
- Negative guard rows still fail closed for their original owner.
- Backend subset proof passes.

## Step 3: Broaden Within The Same Fragment Family

Goal: extend support only to adjacent fragment shapes that share the Step 2
authority and lowering model.

Primary target:
- Additional rows in the same refreshed family selected by Step 1 or exposed
  after Step 2.

Actions:
- Re-run focused residual probes after Step 2.
- Identify adjacent same-family shapes with complete upstream facts.
- Add narrowly scoped consumer handling for those shapes.
- Keep cast, select, floating-only, vector, producer-owned, and policy-owned
  rows out of scope unless they are the selected family.

Completion check:
- The selected family is materially reduced or reclassified to concrete
  downstream owners.
- Negative proof still covers missing producer/prepared-authority rows.
- Backend subset proof passes.

## Step 4: Next-Family Or Close-Readiness Classification

Goal: decide whether idea 612 needs another instruction-fragment family packet,
a regenerated runbook, a split follow-up, or closure.

Actions:
- Refresh the remaining unsupported instruction-fragment residual set.
- Confirm whether remaining rows are still RV64 consumer-owned or belong to
  producer, prepared-authority, select, floating/vector/policy, ABI, global, or
  runtime owners.
- Record close readiness, next-family recommendation, or split follow-up need
  in `todo.md`.

Completion check:
- `todo.md` states whether idea 612 is close-ready, should continue with a
  named next family, or should split/rewrite the route.
- The recommendation is backed by current diagnostics and backend subset proof
  when code changed in the route.
