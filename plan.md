# Mixed Local/Global Publication Authority Runbook

Status: Active
Source Idea: ideas/open/640_mixed_local_global_publication_authority.md

## Purpose

Classify and repair the prepared publication authority needed for residual rows
that mix ordinary local memory, global memory, pointer traffic, freshness, or
publication ordering.

## Goal

Identify one shared mixed local/global publication authority family and move it
past its current first owner, or split the rows into more precise source ideas
with current evidence.

## Core Rule

Do not broaden direct global-symbol local-memory policy. This plan owns only
mixed local/global publication authority proven by refreshed diagnostics.

## Read First

- `ideas/open/640_mixed_local_global_publication_authority.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `ideas/closed/608_prepared_global_data_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

## Current Targets

Representative residual rows from idea 631 Step 5:

- `src/pr57861.c`
- `src/pr58431.c`
- `src/pr68185.c`
- `src/pr68321.c`
- `src/pr70005.c`

## Non-Goals

- Do not reopen direct global-symbol local-memory support from idea 631.
- Do not absorb prepared global value-location consumption from idea 621.
- Do not absorb aggregate stack-home policy from idea 633.
- Do not absorb move-bundle fan-in authority from idea 637.
- Do not change runtime/library policy, expectations, unsupported markers,
  allowlists, timeouts, or accounting.
- Do not claim progress through helper renames or diagnostic wording changes.

## Working Model

The residual rows have scalar global-memory facts, but their first remaining
blocker may be ordinary local publication, global publication, pointer
freshness, aggregate ownership, publication ordering, or a focused RV64
consumer rule. Execution must refresh current evidence before choosing a
producer or consumer packet.

## Execution Rules

- Refresh diagnostics before implementing a repair.
- Classify each row by the first missing authority fact, not by filename or
  historical bucket.
- Prefer prepared/prealloc producer authority when the missing fact is not
  already explicit.
- Add RV64 consumer support only for a shared authority shape already proven by
  prepared facts.
- Keep stale pointer values, ambiguous publication order, missing
  local/global ownership, and aggregate-lane mismatches fail-closed.
- Preserve negative proof that direct global-symbol local-memory, ordinary
  global memory, aggregate homes, and runtime-only failures remain outside this
  idea.

## Ordered Steps

### Step 1: Refresh Mixed Publication Residuals

Goal: establish the current first owner for each representative row.

Primary targets: `src/pr57861.c`, `src/pr58431.c`, `src/pr68185.c`,
`src/pr68321.c`, and `src/pr70005.c`.

Actions:

- Run focused diagnostics for the representative rows from a fresh build.
- Record the first missing fact for each row as one of local publication,
  global publication, pointer freshness, publication ordering, aggregate
  ownership, RV64 consumer rule, or unrelated owner.
- Compare each refreshed stop against the closed direct global-symbol
  local-memory route from idea 631.
- Leave rows outside mixed local/global publication fail-closed and identify
  their existing or new owner.

Completion check:

- `todo.md` records the refreshed owner classification and the supervisor has a
  single selected family for the next implementation packet, or a documented
  split recommendation if no shared family exists.

### Step 2: Publish One Shared Prepared Authority

Goal: add the first explicit producer fact for the selected mixed publication
family.

Primary targets: prepared/prealloc publication, freshness, or ordering
producer code identified by Step 1.

Actions:

- Locate the producer layer that already owns the selected source facts.
- Publish only the authority needed by the selected family.
- Add focused positive and negative coverage for one legal mixed-publication
  shape and one missing-authority rejection.
- Preserve diagnostics for stale pointer values, ambiguous order, missing
  ownership, and aggregate-lane mismatches.

Completion check:

- A narrow build and focused tests prove the selected prepared authority is
  present for the legal shape and absent for missing or ambiguous authority.

### Step 3: Consume Explicit Authority In RV64 If Needed

Goal: consume the selected prepared authority in RV64 only after Step 2 proves
the fact exists.

Primary targets: RV64 local/global memory consumer code identified by Step 1
or Step 2.

Actions:

- Locate the RV64 consumer path that rejects the selected mixed-publication
  shape.
- Consume explicit prepared authority; do not infer authority from source
  order, final assembly order, diagnostics, or testcase identity.
- Add a focused backend assertion for the legal consumer shape.
- Keep unsupported or mismatched authority states rejected.

Completion check:

- The selected row or focused backend case advances past its previous RV64
  first owner, while missing, stale, ambiguous, or mismatched authority remains
  fail-closed.

### Step 4: Reclassify Residual Rows And Validate

Goal: prove the implemented family is not a testcase-shaped shortcut and
preserve accurate owners for remaining rows.

Primary targets: all representative residual rows and nearby same-feature
negative cases found during execution.

Actions:

- Rerun the representative residual rows and compare first owners.
- Record rows that now advance, rows that remain blocked by the same selected
  family, and rows that belong to another existing or new source idea.
- Run the supervisor-selected broader validation checkpoint if the producer or
  RV64 consumer change affects shared memory publication paths.
- Keep expectation, unsupported-marker, allowlist, timeout, runtime, and
  accounting files unchanged unless the supervisor explicitly authorizes a
  non-capability maintenance slice.

Completion check:

- At least one shared mixed-publication family moves past its current owner, or
  the route creates precise follow-up ideas with current evidence; negative
  proof keeps out-of-scope owners outside this plan.
