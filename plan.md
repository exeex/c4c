# Phase F5 Memory Accesses Cross-Publication Mismatch Fail-Closed Proof

Status: Active
Source Idea: ideas/open/277_phase_f5_memory_accesses_cross_publication_mismatch_fail_closed_proof.md

## Purpose

Prove one real-target `PreparedFunctionLookups::memory_accesses`
cross-publication mismatch row fails closed when the prepared/public memory row
belongs to a different owner or publication than the Route 3 / Route 5 semantic
authority.

## Goal

Add one bounded backend proof that rejects a wrong owner/publication
`memory_accesses` row while preserving the compatible positive output.

## Core Rule

Do not claim cross-publication coverage through helper-only classification,
expectation rewrites, unsupported downgrades, or testcase-shaped shortcuts. The
proof must exercise a real target consumer and keep existing fallback,
status/debug, route-debug, prepared-printer, wrapper, exact-output, and baseline
contracts intact.

## Read First

- `ideas/open/277_phase_f5_memory_accesses_cross_publication_mismatch_fail_closed_proof.md`
- Existing Phase F5 `memory_accesses` backend tests and fixtures.
- The Route 3 / Route 5 semantic authority checks adjacent to the selected
  real consumer.

## Scope

- One real target consumer with existing or supportable fixture coverage.
- One prepared memory/source row whose owner or publication mismatches the
  Route 3 / Route 5 authority.
- Status/debug evidence that distinguishes cross-publication mismatch from
  prepared-only rejection, byte-offset drift, and stale-publication rejection.
- Positive-path output and compatibility rows for the selected consumer.

## Non-Goals

- Do not attempt an exhaustive cross-target matrix.
- Do not fold in byte-offset coverage, x86 parity, edge-publication families,
  metadata, liveness, aggregate retirement, or draft 155 work.
- Do not retire, hide, or broadly rewrite prepared APIs.
- Do not treat helper-only classification as backend boundary proof.

## Working Model

The selected fixture should keep Route 3 / Route 5 authority internally
consistent for the compatible path. The negative row should be a normal
prepared/public memory-access row with a different owner or publication from
that authority, so the failure is caused by cross-publication mismatch rather
than offset drift, missing fixture support, or stale-publication rejection.

## Execution Rules

- Keep routine progress in `todo.md`; edit this runbook only for real route
  correction.
- Prefer existing fixture helpers and naming patterns from nearby Phase F5
  tests.
- Before accepting a negative proof, also assert the compatible positive path
  still emits the exact expected output.
- If normal fixture construction rejects the mismatch before the backend
  consumer path runs, record the blocker in `todo.md` instead of adding
  synthetic state mutation.
- Run the supervisor-delegated proof command exactly for executor packets.

## Steps

### Step 1: Locate The Real Consumer Fixture

Goal: identify the narrowest existing `memory_accesses` consumer fixture that
can express owner or publication mismatch without synthetic state mutation.

Primary target: existing backend Phase F5 memory-access publication tests.

Actions:

- Inspect the RISC-V and adjacent backend prepared-edge publication tests that
  already exercise `PreparedFunctionLookups::memory_accesses`.
- Name the compatible target output and the Route 3 / Route 5 authority row.
- Identify whether owner mismatch or publication mismatch is the smallest
  supportable cross-publication row.

Completion check:

- `todo.md` names the chosen test/fixture, the compatible output contract, and
  the exact mismatch kind to prove next.

### Step 2: Add The Cross-Publication Negative Proof

Goal: prove the selected real backend consumer rejects the wrong
owner/publication memory-access row as semantic agreement.

Primary target: the selected backend test file from Step 1.

Actions:

- Construct the mismatched prepared memory/source row using normal fixture
  construction.
- Drive the row through the real backend consumer path, not only a helper or
  oracle.
- Assert the negative status/debug evidence distinguishes
  cross-publication mismatch from prepared-only, byte-offset drift, and stale
  publication cases.
- Assert the wrong owner/publication row is not accepted as Route 3 / Route 5
  agreement.

Completion check:

- The negative test fails closed for the mismatched row with precise
  status/debug evidence and without weakening existing contracts.

### Step 3: Preserve The Compatible Positive Path

Goal: keep the compatible target output and agreement behavior stable.

Primary target: the same selected backend test file.

Actions:

- Assert the compatible row still emits the exact expected target output.
- Assert Route 3 / Route 5 agreement remains true for the compatible row.
- Avoid weakening fallback, wrapper, route-debug, prepared-printer,
  helper/oracle, exact-output, or baseline expectations.

Completion check:

- The test proves both fail-closed mismatch behavior and unchanged compatible
  output in the same real consumer surface.

### Step 4: Run The Targeted Acceptance Proof

Goal: produce fresh build and narrow-test evidence for the bounded proof.

Actions:

- Run the supervisor-delegated build plus targeted CTest command exactly.
- Save the proof in `test_after.log` if delegated as the canonical executor
  proof log.
- Record in `todo.md` that no fallback, status/debug, route-debug,
  prepared-printer, wrapper, helper/oracle, exact-output, or baseline contract
  was weakened.

Completion check:

- The delegated build and targeted test pass, and `todo.md` records the command,
  result, and remaining scope, if any.
