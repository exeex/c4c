# RV64 Scalar Select And Join Materialization Runbook

Status: Active
Source Idea: ideas/open/427_rv64_scalar_select_join_materialization.md

## Purpose

Activate the highest-priority evidence-backed RV64 follow-up from the
instruction-fragment classification work.

Goal: lower coherent prepared scalar `bir.select` and related join-transfer
fragments through RV64 object emission without inventing missing prepared
facts.

## Core Rule

Use existing prepared BIR, value-home, branch-condition, and join-transfer
facts as authority. Do not add filename-shaped paths, expectation changes,
allowlist edits, unsupported-marker downgrades, or pass/fail accounting changes
as proof of progress.

## Read First

- `ideas/open/427_rv64_scalar_select_join_materialization.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `build/agent_state/unsupported_instruction_fragment_rows.tsv` when present
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/prealloc/prepared_object_traversal.hpp`
- `src/backend/prealloc/prepared_object_traversal.cpp`
- `src/backend/prealloc/select_chain_lookups.cpp`

## Current Scope

- Prepared scalar `bir.select` object lowering for ordinary integer and
  pointer-sized scalar results.
- Prepared join-transfer carrier rows that can be emitted from coherent
  prepared join facts.
- Representative evidence rows from the classification bucket:
  `src/pr43236.c`, `src/pr51933.c`, `src/pr68328.c`, `src/pr82954.c`, and
  `src/pr84524.c`.
- Fail-closed diagnostics for incomplete prepared select or join-transfer
  facts.

## Non-Goals

- F128 or long-double select support.
- Aggregate `sret`/`byval`, call publication, inline asm, pointer provenance,
  or global memory repair.
- New prepared branch, dominance, value-home, producer, or join facts in RV64
  lowering.
- Expectation, allowlist, unsupported-marker, runtime comparison, or scan
  accounting edits.
- Broad RV64 object-emission rewrites unrelated to select/join materialization.

## Working Model

- Prepared traversal can already classify ordinary select and prepared
  join-transfer carrier rows.
- `prepared_scalar_emit.cpp` contains scalar select emission helpers for the
  prepared function route.
- The RV64 object route still rejects some coherent select/join fragments with
  `unsupported_instruction_fragment`; this runbook should connect the existing
  semantic select/join authority to object emission.
- The first implementation packets should prove one concrete ordinary scalar
  shape, then generalize to nearby same-feature shapes.

## Execution Rules

- Keep packet boundaries narrow and testable.
- Prefer reusing or extracting existing select emission helpers over duplicating
  ad hoc object-route logic.
- Preserve diagnostics for missing, ambiguous, malformed, or unsupported
  prepared join-transfer data.
- Any successful proof must include more than one select/join shape before the
  source idea can be treated as complete.
- Escalate to plan review if the route needs producer metadata not already
  present in prepared facts.

## Step 1: Inspect Evidence And Select A First Owned Shape

Goal: identify the smallest coherent prepared select/join row family that the
executor can repair without route drift.

Concrete actions:

- Inspect the select/join bucket rows in
  `build/agent_state/unsupported_instruction_fragment_rows.tsv` if the file is
  present; otherwise use the representative rows in the docs.
- Reproduce or inspect the `src/pr43236.c` prepared scalar `bir.select` row and
  identify the object-route rejection point.
- Compare prepared-function select handling with object-emission select/join
  handling, especially the classifications from
  `classify_prepared_object_select_consumer`.
- Record the first implementation packet target in `todo.md`, including the
  exact proof command delegated by the supervisor.

Completion check:

- `todo.md` names a concrete first implementation packet, target files, row
  family, and narrow proof command.
- No implementation files are changed in this step unless the supervisor has
  delegated execution beyond planning.

## Step 2: Emit One Coherent Scalar Select In The Object Route

Goal: make one ordinary prepared scalar select row lower through RV64 object
emission by semantic rule.

Concrete actions:

- Use prepared value homes/registers and existing compare/select emission
  helpers where possible.
- Emit only supported integer or pointer-sized scalar result types.
- Preserve fail-closed behavior for missing result homes, unsupported result
  types, unsupported predicates, or unavailable operands.
- Add focused regression coverage for the repaired shape.

Completion check:

- The target row no longer fails with `unsupported_instruction_fragment`.
- A nearby unsupported/incomplete select still fails closed with a clear
  diagnostic.
- Fresh build or compile proof and the supervisor-delegated narrow test subset
  pass.

## Step 3: Generalize To Join-Transfer Carrier Rows

Goal: handle prepared join-transfer carrier classifications without adding
case-specific shortcuts.

Concrete actions:

- Use prepared join-transfer classification as authority for carrier rows.
- Map coherent carrier rows to the same scalar materialization path where the
  value homes and branch facts are complete.
- Keep missing, ambiguous, malformed, or unsupported join-transfer rows
  rejected by existing diagnostic categories.
- Add at least one focused test distinct from the Step 2 shape.

Completion check:

- At least one prepared join-transfer carrier row lowers semantically.
- Ordinary select and join-transfer tests pass together.
- Diagnostics still distinguish incomplete prepared facts from unsupported
  lowering.

## Step 4: Prove Nearby Same-Feature Coverage

Goal: show the route is semantic across the select/join bucket, not shaped to
one file.

Concrete actions:

- Exercise representative rows from at least two of `src/pr43236.c`,
  `src/pr51933.c`, `src/pr68328.c`, `src/pr82954.c`, and `src/pr84524.c`, as
  feasible under the supervisor-selected subset.
- Inspect remaining select/join `unsupported_instruction_fragment` rows and
  classify any true out-of-scope leftovers in `todo.md`.
- Do not fold call-adjacent, aggregate ABI, pointer provenance, global memory,
  or F128 failures into this route.

Completion check:

- Focused tests cover multiple select/join shapes through one semantic path.
- Any remaining select/join unsupported rows have an explicit reason that is
  either incomplete prepared data or out of scope for this source idea.

## Step 5: Broader Validation And Close Readiness

Goal: prepare the supervisor to decide whether the source idea is complete.

Concrete actions:

- Run the supervisor-delegated broader RV64 or gcc_torture subset after the
  focused implementation packets are green.
- Summarize changed behavior, remaining diagnostics, and any select/join rows
  still failing.
- Confirm no expectation, allowlist, unsupported-marker, runtime comparison, or
  pass/fail accounting changes were used as progress.

Completion check:

- Build and selected regression proof are fresh.
- `todo.md` summarizes completion evidence and remaining scoped risks.
- The source idea acceptance criteria can be evaluated without re-reading the
  whole diff.
