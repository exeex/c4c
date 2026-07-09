# RV64 Cast Residual Classifier Runbook

Status: Active
Source Idea: ideas/open/623_rv64_cast_instruction_fragment_consumers.md

## Purpose

Keep idea 623 on a cast-consumer route while preventing more RV64 consumer
widening until the Step 4 no-breadth evidence is explained.

## Goal

Classify why the remaining width-preserving cast rows did not benefit from the
Step 3 semantic move repair, separate the `src/pr81556.c` runtime mismatch,
and only then choose the next implementation packet.

## Core Rule

Do not add another RV64 cast consumer helper or widen the existing helper until
the remaining 18 zext rows, 3 trunc rows, and `src/pr81556.c` runtime mismatch
are classified by first failing owner and shared semantic cause.

## Read First

- `ideas/open/623_rv64_cast_instruction_fragment_consumers.md`
- `review/idea623_cast_breadth_review.md`
- `todo.md`
- `build/agent_state/623_step1_cast_residuals.tsv`
- `build/agent_state/623_step2_cast_owner_buckets.tsv`
- `build/agent_state/623_step4_cast_guard_notes.md`
- `build/agent_state/623_step4_cast_guard_summary.tsv`

## Current Targets

- The narrowed Step 5 residuals that must be classified before more RV64 cast
  consumer widening:
  - 18 `rv64-consumer:width-preserving-zext-i32-to-i32`
  - 3 `rv64-consumer:width-preserving-trunc-i32-to-i32`
  - `src/pr81556.c`, which moved to `RV64_BACKEND_RUNTIME_MISMATCH`
- The existing Step 3 same-width `ZExt`/`Trunc` i32 GPR move helper as context,
  not as proof of broad bucket closure.
- The Step 4 non-cast guard set, only to confirm the route boundary remains
  closed.
- Other Step 4 cast residual buckets, including ptrtoint and floating-policy
  rows, remain recorded evidence but are not the next widening target.

## Non-Goals

- More RV64 cast-consumer implementation before Step 5 classification is
  complete.
- Reverting the Step 3 implementation solely because breadth failed.
- BIR semantic cast production, pointer `BinaryInst`, scalar narrow-integer,
  move-bundle, terminator, ABI, runtime, local-memory, select, branch, global,
  expectation, unsupported-marker, allowlist, timeout, or accounting changes.
- Treating `src/pr81556.c` as another `CastInst` unsupported row after it has
  reached a runtime mismatch.
- Floating, vector, or library policy cast lanes unless refreshed evidence
  proves ordinary-C backend leverage and this runbook is narrowed again.

## Working Model

- Step 1 refreshed cast residual evidence.
- Step 2 bucketed candidate cast ownership.
- Step 3 added a narrow semantic same-width i32 GPR copy repair for `ZExt` and
  `Trunc`.
- Step 4 proved that repair is useful but not broad: one saved cast row passed,
  21 width-preserving rows remained, and `src/pr81556.c` changed failure class.
- Step 4 also left ptrtoint and floating-policy residuals that should remain
  in their owner lanes unless later lifecycle work explicitly narrows to them.
- The next packet is classifier work, not implementation work.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit `ideas/open/623_rv64_cast_instruction_fragment_consumers.md`
  unless durable source intent changes.
- Do not edit implementation files during Step 5.
- Do not downgrade supported-path expectations or weaken test contracts.
- Do not claim progress through expectation, unsupported-marker, allowlist,
  timeout, or accounting rewrites.
- Preserve the Step 4 non-cast guard evidence as a route boundary.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.
- Escalate to supervisor/reviewer if classification shows the only available
  path is a single named-case shortcut or a broad non-cast route.

## Ordered Steps

### Step 1: Refresh cast residual evidence

Status: Complete.

Completion evidence:
- Step 1 refresh artifacts recorded current cast-shaped
  `unsupported_instruction_fragment` rows and nearby non-cast guard rows.
- No implementation files were changed.

### Step 2: Split ownership by first missing fact

Status: Complete.

Completion evidence:
- Step 2 bucketed candidate rows by owner and identified width-preserving
  `ZExt`/`Trunc` i32-to-i32 rows as a candidate RV64 consumer family.
- Non-RV64 owner rows remained outside this route.

### Step 3: Implement one verified cast consumer sub-family

Status: Complete as narrow semantic progress, not broad bucket closure.

Completion evidence:
- The Step 3 helper accepts semantic same-width i32 `ZExt`/`Trunc` GPR shapes,
  emits an RV64 move, and fails closed for unsupported opcode, type, and home
  combinations.
- Reviewer report `review/idea623_cast_breadth_review.md` judged this not to
  be testcase-overfit.

### Step 4: Prove breadth and guardrails

Status: Complete with no-breadth blocker.

Completion evidence:
- Step 4 reran the saved cast residual rows and nearby non-cast guard rows.
- Only `src/p18298.c` passed from the saved cast set.
- 18 zext rows and 3 trunc rows still failed as width-preserving cast
  residuals, and `src/pr81556.c` reached `RV64_BACKEND_RUNTIME_MISMATCH`.
- Other cast residuals remained in ptrtoint and floating-policy buckets and are
  not targets for immediate consumer widening.
- All 60 non-cast guard rows remained failed in their non-cast owner classes.

### Step 5: Classify Step 4 no-breadth residuals

Goal: Explain why the remaining width-preserving rows and `src/pr81556.c`
did not establish breadth before any further cast-consumer implementation.

Primary target:
- Step 4 residual artifacts under `build/agent_state/623_step4_*`, plus the
  matching Step 1 and Step 2 row data.

Concrete actions:
- List the 18 remaining zext rows and 3 remaining trunc rows with source file,
  operation, Step 2 owner bucket, Step 4 failure class, and emitted diagnostic.
- For each row, classify the first still-missing fact or responsible owner:
  consumer shape mismatch, operand/result type mismatch, non-GPR home,
  missing prepared value, source-kind/authority gap, select-edge dependency,
  producer semantic gap, or other owner.
- Compare the failing width-preserving rows against the passing `src/p18298.c`
  row and the Step 3 helper acceptance conditions.
- Classify `src/pr81556.c` separately as a runtime-mismatch investigation
  candidate, including whether object compilation now succeeds and where the
  runtime value diverges if evidence already exists.
- Confirm the 60 non-cast guard rows are used only as boundary evidence and do
  not become implementation targets.

Completion check:
- `todo.md` records a residual table or artifact path covering all 18 zext
  rows, all 3 trunc rows, and `src/pr81556.c`.
- Each residual has a first-owner classification and a short reason.
- The packet changes no implementation files and makes no expectation,
  unsupported-marker, allowlist, timeout, or accounting changes.
- The next implementation candidate, if any, is named as a semantic multi-row
  cause rather than a source-file shortcut.

### Step 6: Choose the next route from classification

Goal: Decide whether idea 623 still has an RV64 cast-consumer packet or should
hand residuals to another owner route.

Concrete actions:
- If Step 5 finds a shared multi-row RV64 consumer cause with complete upstream
  facts, narrow this runbook to that cause before implementation.
- If Step 5 shows producer, authority, ABI, select-edge, runtime, or policy
  ownership, record that owner and stop cast-consumer widening.
- If only a single named case remains actionable, route it through supervisor
  review before any code work.

Completion check:
- `todo.md` names the selected next packet shape or records a route blocker.
- Supervisor has enough evidence to delegate an executor packet without
  rediscovering the Step 4 failure family.

### Step 7: Implement only a classified multi-row cast-consumer cause

Goal: Repair a classified cast capability only after Steps 5 and 6 prove it is
inside idea 623's RV64 consumer scope.

Concrete actions:
- Implement the narrow semantic rule selected by Step 6.
- Preserve fail-closed diagnostics for rows still missing upstream facts.
- Keep unrelated pointer, ABI, local-memory, select, branch, global, runtime,
  move-bundle, terminator, expectation, unsupported-marker, allowlist, timeout,
  and accounting behavior unchanged.

Completion check:
- Fresh build or compile proof is recorded in `test_after.log`.
- Rerun evidence shows multi-row cast progress for the classified cause, or
  `todo.md` records a justified no-breadth blocker.
- Non-cast guard rows remain outside the implementation route.
