# RV64 Pointer BinaryInst Address-Authority Runbook

Status: Active
Source Idea: ideas/open/612_rv64_instruction_fragment_consumers.md
Supersedes: exhausted narrow integer BinaryInst route through Step 4.

## Purpose

Continue idea 612 only for the remaining in-scope pointer `BinaryInst` /
address-authority consumer family after the narrow integer add-immediate route
was classified as exhausted.

## Goal

Repair RV64/MIR instruction-fragment consumption for pointer `BinaryInst` rows
only when upstream pointer or address authority is complete, while preserving
fail-closed diagnostics for producer-owned, prepared-authority, ABI, select,
branch, move-bundle, global, runtime, policy, and singleton narrow-integer rows.

## Core Rule

Consume explicit pointer/address facts; do not infer pointer base, offset,
freshness, frame-slot, local-memory, or selected-authority facts from final
object shape, and do not use the singleton scalar `ashr` row as a standalone
implementation target.

## Read First

- `ideas/open/612_rv64_instruction_fragment_consumers.md`
- `ideas/open/614_rv64_pointer_local_memory_consumption.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
- `todo.md`

## Current Targets

- Rows whose first reproduced stop is still
  `unsupported_instruction_fragment` with pointer `BinaryInst` ownership.
- Pointer arithmetic or address-authority rows where refreshed evidence shows
  complete upstream facts and an RV64/MIR consumer stop.
- Negative guard rows from the Step 4 classification:
  - singleton scalar narrow integer `src/931110-1.c` / `ashr`
  - cast producer or cast consumer rows
  - call/ABI and policy rows
  - select publication rows
  - inline asm, terminator, move-bundle, branch freshness, global-data,
    local-memory producer, stack-frame, parameter-home, and runtime rows

## Non-Goals

- Do not implement the singleton `src/931110-1.c` scalar `ashr` row without
  refreshed same-family breadth.
- Do not repair BIR pointer/address production, BIR cast production, local or
  global memory producer semantics, selected pointer authority publication,
  ABI lowering, select publication, branch stack-source freshness, move-bundle
  destination authority, runtime behavior, expectations, unsupported markers,
  allowlists, timeout/accounting, or GCC torture classification metadata.
- Do not fold idea 614 pointer local-memory consumption into this runbook
  unless refreshed diagnostics prove the row is first blocked at generic
  pointer `BinaryInst` instruction-fragment consumption rather than
  local-memory authority consumption.
- Do not add filename-, row-, operation-id-, or testcase-shaped shortcuts.

## Working Model

- Step 4 classified the narrow integer route as exhausted after add-immediate
  work. The only same-family scalar integer row left visible was the
  `src/931110-1.c` `ashr` singleton, which is not enough breadth for another
  narrow integer packet.
- The plausible remaining idea-612 family is pointer `BinaryInst` /
  address-authority. Treat the 23 plain pointer `BinaryInst` rows and broader
  pointer arithmetic/address-authority rows from stale diagnostics as planning
  candidates until refreshed probes prove current ownership.
- Existing open ideas already own many residual families:
  - idea 613: ABI/call/result/stack-frame lowering
  - idea 614: pointer local-memory consumption of selected authority
  - idea 615: branch stack-source freshness audit and repair
  - idea 616: select publication source wiring
  - idea 617: scalar compare publication
  - idea 618: runtime mismatch ownership investigation
  - idea 619 and idea 621: global-data and prepared-global handoff/consumer
  - idea 622: repeated stack-destination fan-in authority
- Cast rows remain outside this regenerated pointer route unless a later
  lifecycle decision creates or activates a cast-specific idea.

## Execution Rules

- Start from current diagnostics and focused direct probes, not stale counts.
- Before any code-changing packet, record in `todo.md` the selected pointer
  row family, at least two positive candidates when available, negative guard
  rows, and the exact proof command.
- Each code-changing step must run:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Use focused direct object probes for representative positive and negative
  rows when backend logs are stale or ambiguous.
- Escalate to reviewer if a diff weakens unsupported contracts, rewrites
  expectations, moves producer repair into RV64, consumes selected/local-memory
  authority without complete facts, or proves progress only by matching a named
  source file.

## Step 1: Refresh Pointer BinaryInst Ownership

Goal: identify the current pointer `BinaryInst` / address-authority population
that still belongs to idea 612.

Primary target:
- RV64 backend-object logs and focused probes for pointer-shaped
  `unsupported_instruction_fragment` rows.

Actions:
- Refresh or inspect current backend diagnostics for rows previously recorded
  as plain pointer `BinaryInst`, pointer arithmetic, or address-authority
  instruction-fragment candidates.
- Split candidates by first owner:
  - RV64 pointer `BinaryInst` consumer with complete upstream pointer/address
    facts
  - BIR pointer/address producer gap
  - selected pointer or local-memory authority gap owned by idea 614
  - branch stack-source, select publication, ABI/call, move-bundle, global,
    runtime, inline asm, terminator, cast, or other policy owner
- Include `src/931110-1.c` only as a negative guard unless refreshed evidence
  finds broader same-family scalar `ashr` rows.
- Record the selected Step 2 packet, positive rows, negative guard rows, and
  exact proof command in `todo.md`.

Completion check:
- `todo.md` names the refreshed pointer groups, the selected Step 2 pointer
  family, positive candidates or a clear no-breadth blocker, negative guard
  rows including the singleton `ashr`, and the exact proof command.

## Step 2: Implement First Pointer Address-Authority Consumer

Goal: lower one general pointer `BinaryInst` consumer family with explicit
upstream address authority.

Primary target:
- RV64/MIR object-emission instruction lowering path selected by Step 1.

Actions:
- Locate the RV64 consumer rejection for the selected pointer fragment family.
- Add the smallest semantic lowering rule shared by the selected rows.
- Require complete upstream pointer/address facts, operand sources, widths,
  base/offset authority, and local/global/selected-authority boundaries.
- Preserve rejection for missing producer facts, ambiguous selected authority,
  unsupported pointer arithmetic, local-memory producer gaps, unsupported
  widths, and out-of-scope fragment classes.
- Avoid expectation, unsupported-marker, allowlist, timeout, runtime, and
  accounting changes.

Completion check:
- Multiple pointer rows from the selected family compile or move past
  `unsupported_instruction_fragment`.
- Negative guard rows still fail closed under their original owners.
- Backend subset proof passes.

## Step 3: Broaden Only Within Pointer BinaryInst Authority

Goal: extend support only to adjacent pointer `BinaryInst` shapes that share
the Step 2 authority and lowering model.

Primary target:
- Additional pointer rows in the same refreshed family selected by Step 1 or
  exposed after Step 2.

Actions:
- Re-run focused residual probes after Step 2.
- Identify adjacent pointer shapes with complete upstream authority and the
  same consumer semantics.
- Add narrowly scoped consumer handling for those shapes.
- Keep cast, scalar narrow integer, select, branch, ABI, move-bundle, global,
  runtime, producer-owned, and policy-owned rows out of scope.

Completion check:
- The selected pointer family is materially reduced or reclassified to
  concrete downstream owners.
- Negative proof still covers missing producer/prepared-authority rows and the
  singleton `ashr` guard.
- Backend subset proof passes.

## Step 4: Residual Split Or Close-Readiness Classification

Goal: decide whether idea 612 has another instruction-fragment consumer family
left, should split residuals into separate open ideas, or is close-ready.

Actions:
- Refresh remaining `unsupported_instruction_fragment` residuals after pointer
  work.
- Classify residuals against existing open idea owners listed in the working
  model.
- Identify whether cast producer/consumer rows require a new durable open idea
  or can stay as a later idea-612 runbook family.
- Keep the singleton `ashr` row out of implementation unless broader
  same-family evidence appears.
- Record close readiness, next-family recommendation, or split follow-up need
  in `todo.md`.

Completion check:
- `todo.md` states whether idea 612 is close-ready, should continue with a
  named next family, or should split/retire the route.
- The recommendation is backed by current diagnostics and backend subset proof
  when code changed in the route.
