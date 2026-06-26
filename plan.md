# RV64 Object Route Instruction Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md

## Purpose

Repair the dominant RV64 prepared-object bucket where prepared BIR
instructions reach object emission but are rejected as unsupported instruction
fragments.

## Goal

Make reusable prepared instruction-fragment families lower to valid RV64
object code without reconstructing BIR ownership inside the target emitter.

## Core Rule

RV64 object emission may consume explicit prepared instruction, value-home,
and operand facts. It must not special-case torture filenames, weaken
expectations, or infer missing BIR control/data-flow semantics from source or
instruction shape.

## Read First

- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/closed/406_rv64_object_route_residual_local_memory_boundaries.md`
- `tests/c/external/gcc_torture/src/20000223-1.c`
- `tests/c/external/gcc_torture/src/20020225-2.c`
- Current RV64 gcc_torture backend logs and prepared dumps for instruction
  fragment representatives

## Current Targets

- The 2026-06-26 reopened 354 classification found 385 failures with
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- Primary representative: `src/20000223-1.c`.
- Residual routed from idea 406: `src/20020225-2.c` now advances past local
  memory and stops at the same instruction-fragment owner.
- Nearby same-fragment cases should be selected from current backend artifacts
  before accepting a lowering packet.

## Non-Goals

- Do not repair stack-frame, callee-saved, parameter-home, or variadic
  admission work owned by
  `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`.
- Do not repair terminator-fragment, move-bundle, global-data, runtime
  mismatch, or wide-rematerialized-immediate producer work owned by other open
  ideas.
- Do not treat semantic `lir_to_bir` failures as instruction-fragment
  lowering.
- Do not rewrite gcc_torture expectations, change allowlists to hide failures,
  or mark supported-path cases unsupported.
- Do not add filename-specific lowering for `20000223-1.c`, `20020225-2.c`,
  or any other representative.

## Working Model

This idea owns target-side lowering for prepared BIR instructions whose
semantic facts are already published enough for RV64 object emission. Each
packet should first classify the rejected instruction family and then lower a
reusable instruction form with proof from at least one representative and one
nearby same-fragment case when available.

If classification shows that a failure lacks required producer facts, stop and
route that boundary to a producer-side idea instead of teaching RV64 object
emission to rediscover those facts.

## Execution Rules

- Keep each implementation packet tied to one concrete instruction-fragment
  family.
- Inspect prepared dumps before changing RV64 lowering; record operand widths,
  homes, immediates, memory facts, and result publication requirements.
- Preserve the prepared-object contract boundary. Do not move BIR ownership,
  value-home selection, or CFG reconstruction into target code.
- Add or update focused backend coverage only where it proves the semantic
  lowering family, not just a representative filename.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat diagnostic-only churn, expectation rewrites, and single-case green
  proof as insufficient progress.

## Step 1: Classify Instruction Fragment Families

Goal: identify the concrete unsupported prepared instruction families behind
the current bucket and select the first reusable lowering packet.

Actions:

- Reproduce or inspect the current RV64 gcc_torture backend artifacts for
  `src/20000223-1.c`, the 406 residual `src/20020225-2.c`, and nearby
  `unsupported_instruction_fragment` cases.
- Inspect prepared dumps for each selected representative and record opcode,
  operand types, result width, value-home facts, immediates, and any memory or
  publication requirements.
- Map each rejected instruction to the RV64 object-route code path that emits
  the unsupported instruction diagnostic.
- Group cases by reusable instruction family rather than by source filename.
- Decide whether the first packet is a valid target-emission lowering gap or a
  producer-fact gap that needs lifecycle routing.

Completion check:

- `todo.md` names the first concrete instruction-fragment family to repair,
  its representatives, and the exact proof command delegated by the
  supervisor.
- Any producer-fact gap is stopped and routed for lifecycle review instead of
  patched in RV64 object emission.

## Step 2: Lower The First Reusable Instruction Family

Goal: implement semantic RV64 object lowering for the first classified
instruction-fragment family.

Actions:

- Update the RV64 object instruction route to consume explicit prepared facts
  for the selected family.
- Preserve existing width, signedness, immediate, register-home, stack-home,
  and memory-admission checks.
- Keep invalid or unsupported operand shapes rejected with precise diagnostics.
- Add or update focused backend tests when the repo has a matching prepared
  object test surface for the family.

Completion check:

- The selected representative no longer fails with the same
  `unsupported_instruction_fragment` diagnostic for the repaired family.
- A nearby same-fragment case either passes the same boundary or is classified
  as a distinct family with evidence in `todo.md`.
- Existing backend tests selected by the supervisor still pass.

## Step 3: Refresh Bucket Counts And Route The Next Family

Goal: prove the first lowering reduced the current bucket and identify the
next highest-value instruction-fragment family.

Actions:

- Run the supervisor-selected refreshed RV64 gcc_torture backend subset or
  temporary allowlist probe.
- Compare unsupported instruction-fragment diagnostics before and after the
  repair.
- Classify any newly exposed later failure for the repaired representatives as
  same-family, later instruction-family, another open owner, runtime mismatch,
  or producer-fact gap.
- Select the next instruction-fragment family only if it remains inside idea
  395 scope.

Completion check:

- `todo.md` records exact proof results and whether the first family is
  complete, needs another executor packet, or should be split.
- The refreshed subset shows fewer failures in the repaired instruction
  family without introducing new runtime mismatches.

## Step 4: Repeat Family Packets Until 395 Is Exhausted Or Needs Review

Goal: continue reducing reusable instruction-fragment families without drifting
into adjacent owners.

Actions:

- Repeat Step 1 through Step 3 for the next classified instruction-fragment
  family.
- Escalate to plan review if the bucket splits into unrelated producer
  defects, if the current runbook no longer describes the remaining failures,
  or if repeated packets collide with the same unresolved boundary.
- Keep residual stack/frame/home, terminator, move-bundle, global-data,
  runtime, and wide-immediate failures routed to their existing owners.

Completion check:

- All reachable 395-scope representative families either lower semantically,
  have precise residual owner routing, or have a lifecycle review artifact
  explaining why the runbook should be replaced.
- No accepted packet relies on testcase-shaped matching, expectation
  downgrades, allowlist filtering, or diagnostic renaming.
