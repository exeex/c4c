# x86/RISC-V Prepared Edge Publication Handoff Runbook

Status: Active
Source Idea: ideas/open/19_x86_riscv_prepared_edge_publication_handoff.md

## Purpose

Validate that the shared prepared edge-publication authority is target-neutral
enough for future x86 and RISC-V consumers, without expanding into a full
non-AArch64 backend rewrite.

## Goal

Produce a minimal, auditable handoff path or readiness report showing how x86
and RISC-V should consume shared prepared edge-publication facts.

## Core Rule

Shared prepare may describe what value flows across a CFG edge; each target
must still own how to emit that value, including scratch selection, clobber
avoidance, register names, and assembly spelling.

## Read First

- `ideas/open/19_x86_riscv_prepared_edge_publication_handoff.md`
- The shared prepared edge-publication records and helpers introduced by the
  previous BIR edge value-flow work.
- Current x86 and RISC-V lowering surfaces that would need edge or block-entry
  value publication facts.
- The AArch64 consumer path only as a contract example, not as proof of
  target-neutral behavior.

## Current Targets

- Shared prepared edge-publication facts and helper APIs.
- x86 lowering surfaces that will need edge or block-entry publication.
- RISC-V lowering surfaces that will need edge or block-entry publication.
- Focused tests or verifier coverage proving the shared facts do not encode
  AArch64-specific emission policy.

## Non-Goals

- Do not implement full x86 or RISC-V codegen.
- Do not rewrite non-AArch64 control-flow lowering broadly.
- Do not move target-specific emission details into shared prepare or BIR.
- Do not create a second x86/RISC-V-specific edge-copy authority.
- Do not reopen the `00181` runtime repair except as a hazard lesson for the
  target-local consumer boundary.

## Working Model

- Shared prepared facts are the semantic authority for edge/block-entry value
  flow.
- Target lowerers are the emission authority for registers, scratches,
  clobbers, moves, and assembly syntax.
- A minimal non-AArch64 proof may be either an implemented consumer slice or a
  precise readiness report when the target lowerer lacks the needed surface.

## Execution Rules

- Prefer audits and narrow probes before modifying target code.
- If a non-AArch64 consumer slice is attempted, keep it limited to one target
  and one prepared-fact consumption path.
- Reject testcase-shaped shortcuts and named-case-only fixes.
- Any proof must distinguish target-neutral prepared facts from target-local
  emission policy.
- Source-idea edits are not required unless the durable intent changes.

## Step 1: Audit Shared Prepared Edge Publication

Goal: identify whether shared prepared edge-publication records or helpers
carry hidden AArch64 assumptions.

Primary Target: shared prepared edge-publication data structures, builders, and
query helpers.

Actions:

- Inspect prepared fact names, fields, ownership, and construction sites.
- Check for embedded AArch64 register names, scratch policy, assembly spelling,
  or target-specific movement assumptions.
- Note which fields are semantic edge-flow authority and which concerns must
  stay target-local.
- Record any required contract clarification in `todo.md`; only change
  `plan.md` if the runbook itself proves inaccurate.

Completion Check:

- The executor can state whether the shared prepared contract is already
  target-neutral or name the exact hidden AArch64 assumptions that block
  x86/RISC-V consumption.

## Step 2: Audit x86 and RISC-V Consumer Surfaces

Goal: find where non-AArch64 lowerers should consume prepared edge-publication
facts, and whether those surfaces are ready for a minimal proof.

Primary Target: x86 and RISC-V backend lowering/codegen entry points related to
CFG edges, block entry, branch emission, and value moves.

Actions:

- Inventory the current x86 lowering surface for edge or block-entry value
  publication needs.
- Inventory the current RISC-V lowering surface for the same needs.
- Identify whether either target has enough infrastructure for a narrow
  prepared-fact consumer slice.
- Keep target-local responsibilities explicit: scratch selection, clobber
  discipline, register classes, and assembly syntax must remain outside shared
  prepare.

Completion Check:

- The executor can recommend the first viable target for a minimal consumer
  proof, or explain exactly which missing lowerer surface prevents it.

## Step 3: Define Minimal Consumer API Shape

Goal: specify the smallest shared prepared-fact handoff shape expected by x86
and RISC-V consumers.

Primary Target: the boundary between shared prepared facts and target-local
lowering.

Actions:

- Define the query or iteration shape a target lowerer should use to obtain
  edge/block-entry publication facts.
- Keep the API semantic: source value, destination/live-in meaning, edge/block
  identity, and ordering constraints if already present.
- Exclude emission policy: physical register names, temporary allocation,
  target move spelling, and clobber sequencing.
- If the existing API already satisfies the shape, document that fact in
  `todo.md` rather than adding wrapper churn.

Completion Check:

- x86 and RISC-V have a clear minimal path for consuming the same prepared
  edge-publication authority without inventing target-specific duplicate
  authority.

## Step 4: Add Focused Target-Neutral Proof

Goal: add the smallest useful proof that the prepared edge-publication contract
is target-neutral.

Primary Target: verifier/test coverage, or one narrow x86/RISC-V consumer path
if the audited target surface is ready.

Actions:

- Prefer a verifier or unit-style proof that shared prepared facts contain no
  AArch64 register names, scratch policy, or assembly spelling.
- If one non-AArch64 target is ready, add a narrow consumer slice that reads
  prepared facts while leaving emission details target-local.
- If neither target is ready, add a targeted readiness test or report naming
  the exact missing lowerer infrastructure.
- Do not downgrade supported-path tests or weaken expectations to claim
  progress.

Completion Check:

- There is one focused proof: either a working minimal non-AArch64 consumer
  slice or a precise readiness artifact identifying the blocking target
  surface.

## Step 5: Final Handoff and Validation

Goal: make the handoff result durable enough for the supervisor to decide the
next backend initiative.

Primary Target: `todo.md` proof notes plus any accepted test/verifier output
from the implementation steps.

Actions:

- Summarize whether the next initiative should be x86 consumer implementation,
  RISC-V consumer implementation, or another shared prepared contract repair.
- Ensure the final notes name target-local responsibilities that must remain
  outside shared prepare.
- Run the supervisor-delegated proof command for the final implementation
  slice; broader validation is required if the proof touched shared prepared
  authority used by AArch64.

Completion Check:

- The active source idea's acceptance criteria can be evaluated directly from
  the implemented proof, validation output, and final `todo.md` handoff notes.
