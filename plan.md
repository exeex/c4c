# 271 Phase F5 x86/riscv memory_accesses Public-Consumer Fixture Support

Status: Active
Source Idea: ideas/open/271_phase_f5_x86_riscv_memory_accesses_public_consumer_fixture_support.md

## Purpose

Create supported x86 or riscv fixture coverage that reaches a real backend
consumer of the public `PreparedFunctionLookups::memory_accesses` field before
any prepared-only same-consumer fail-closed proof is attempted.

## Goal

Find or add the narrowest supported x86/riscv route where normal prepared
lookup construction reaches a backend consumer that directly reads
`PreparedFunctionLookups::memory_accesses`, while preserving adjacent Route
3/Route 5 compatibility output.

## Core Rule

This is fixture-support work, not the final prepared-only fail-closed proof.
Do not hand-build stale prepared state, rename adjacent edge-publication or
addressing coverage as `memory_accesses` coverage, or weaken any existing
status, debug, printer, wrapper, exact-output, fallback, unsupported, or
baseline contract.

## Read First

- Source idea:
  `ideas/open/271_phase_f5_x86_riscv_memory_accesses_public_consumer_fixture_support.md`
- Prior closed blocker:
  `ideas/closed/270_phase_f5_memory_accesses_prepared_only_same_consumer_fail_closed_proof.md`
- Adjacent x86 fixture named by the source idea:
  `make_x86_param_eq_zero_branch_joined_loadlocal_or_sub_then_add_module()` /
  function `branch_join_loadlocal_then_add`
- Nearest out-of-scope exact public-field consumer:
  `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`

## Current Targets

- A supported x86 or riscv backend path that legitimately consumes
  `PreparedFunctionLookups::memory_accesses`.
- The normal fixture construction path needed to reach that consumer.
- One concrete prepared memory row and its matching Route 3 or Route 5
  authority facts for a later same-consumer prepared-only proof.
- Adjacent x86 Route 3/Route 5 `LoadLocal` compatibility output, preserved as
  adjacent evidence unless the route is changed to consume the public memory
  lookup field.

## Non-Goals

- No final prepared-only fail-closed proof until supported fixture reachability
  exists.
- No hand-built stale prepared state or test-only mutation that bypasses normal
  prepared lookup construction.
- No target widening to AArch64 without a separate lifecycle decision.
- No demotion, deletion, privatization, wrapping, hiding, or renaming of
  `memory_accesses`.
- No expectation weakening, unsupported-status downgrade, baseline churn, or
  route-debug/prepared-printer/wrapper/exact-output/fallback contract changes
  unless the supervisor accepts a reviewed contract update.

## Working Model

- The closed 270 route found no current x86/riscv same-consumer path that
  directly reads the public `memory_accesses` field.
- The adjacent x86 Route 3/Route 5 `LoadLocal` fixture is useful evidence, but
  it is not sufficient unless the target path is changed to consume
  `PreparedFunctionLookups::memory_accesses`.
- Helper and oracle checks can support the route, but they do not establish
  backend fixture support by themselves.
- The deliverable is a supported public-consumer fixture path plus the row and
  authority facts needed by the follow-up fail-closed proof.

## Execution Rules

- Start by mapping actual `PreparedFunctionLookups::memory_accesses` reads in
  x86/riscv code and tests; do not edit implementation before the candidate
  consumer path is named.
- Prefer the narrowest semantic fixture or exposure point that makes normal
  prepared lookup construction reach the public-field consumer.
- Keep fixture changes reusable and semantic; reject named-fixture shortcuts,
  branch-specific matching, and testcase-shaped logic.
- Preserve the adjacent `branch_join_loadlocal_then_add` assembly output unless
  a reviewed contract update explicitly changes it.
- For code-changing steps, run the supervisor-delegated proof command and write
  results to canonical `test_after.log` unless a regression-guard before/after
  packet is explicitly delegated.
- Stop and report a blocker if x86/riscv cannot legitimately consume
  `memory_accesses` without a broader source-idea change.

## Ordered Steps

### Step 1: Map target public-field consumers

Goal: Identify whether any x86 or riscv backend path can legitimately consume
`PreparedFunctionLookups::memory_accesses`.

Primary target: x86/riscv prepared lookup construction, backend lowering, and
tests that mention or could consume `memory_accesses`.

Actions:

- Search for direct reads of `PreparedFunctionLookups::memory_accesses` and
  distinguish backend consumers from helpers, printers, wrappers, and oracles.
- Compare x86 and riscv paths against the nearest exact AArch64 public-field
  consumer only to understand the expected consumer shape.
- Inspect the adjacent x86 Route 3/Route 5 `LoadLocal` fixture and record why
  it currently does or does not reach the public memory lookup field.
- Name the best x86/riscv candidate consumer, or state that none exists without
  source-intent expansion.
- Record the narrow proof command the supervisor should delegate for the next
  packet.

Completion check:

- `todo.md` names the candidate backend consumer, fixture route, adjacent
  compatibility output to preserve, and the narrow proof command.
- If no legitimate x86/riscv candidate exists, `todo.md` records the blocker
  and recommends lifecycle review instead of implementation.

### Step 2: Expose supported fixture reachability

Goal: Make the selected x86/riscv fixture path reach the public-field consumer
through normal prepared lookup construction.

Primary target: The smallest fixture or test-support surface needed by the
consumer selected in Step 1.

Actions:

- Add or expose fixture support for the selected route without synthetic stale
  prepared state.
- Keep helper/oracle checks as support only; include backend consumer evidence
  in the proof.
- Avoid broad rewrites of prepared lookup construction, backend lowering, ABI,
  stack, register, layout, or output policy.
- Preserve adjacent Route 3/Route 5 `LoadLocal` compatibility output unless the
  supervisor accepts a contract change.
- Prefer semantic fixture construction over named-case matching.

Completion check:

- A supported x86 or riscv fixture reaches a backend consumer that directly
  reads `PreparedFunctionLookups::memory_accesses`.
- Matching narrow build/test proof passes for the fixture-support slice.
- The diff does not weaken statuses, expectations, baselines, or existing
  compatibility output.

### Step 3: Record row and authority facts for follow-up proof

Goal: Leave enough concrete evidence for a later same-consumer prepared-only
fail-closed packet to start without rediscovery.

Primary target: `todo.md` execution summary and any narrow test fixture notes
created during Step 2.

Actions:

- Name the concrete prepared memory row reached by the supported fixture.
- Name the matching Route 3 or Route 5 semantic authority facts that the later
  proof should remove or make non-agreeing.
- State which fixture path constructs the row normally.
- State which adjacent x86 compatibility output remains byte-stable.
- Distinguish backend consumer evidence from helper/oracle evidence.

Completion check:

- `todo.md` records the exact consumer, prepared row, authority facts, supported
  fixture path, preserved compatibility output, and follow-up proof command.
- The supervisor can delegate the later fail-closed proof without hand-built
  stale prepared state.

### Step 4: Validate and prepare lifecycle decision

Goal: Prove the fixture-support route is stable enough for supervisor review
and possible source-idea close.

Primary target: Supervisor-selected x86/riscv fixture-support test subset.

Actions:

- Run the delegated narrow proof command for the selected backend path.
- Include adjacent compatibility checks when fixture changes touch Route 3,
  Route 5, helper/oracle, debug, printer, wrapper, or exact-output behavior.
- Escalate to broader validation if the diff touches shared prepared lookup
  construction or cross-backend behavior.
- Do not close the source idea directly; record whether acceptance criteria
  appear satisfied and wait for supervisor-delegated close.

Completion check:

- Narrow proof passes and any requested broader validation has a passing log.
- `todo.md` states whether the source idea appears ready for close, rewrite, or
  follow-up split.
- No testcase-overfit or expectation-weakening reject signal is present.
