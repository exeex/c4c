# Prepared Global Memory Residual Runbook

Status: Active
Source Idea: ideas/open/608_prepared_global_data_authority.md
Activated from: ideas/open/608_prepared_global_data_authority.md
Supersedes: exhausted `Prepared Global Data Authority Runbook` after Step 5

## Purpose

Continue idea `608` after the first runbook moved the broad prepared/global
authority families but found remaining prepared global-memory fact residuals.

## Goal

Repair or precisely classify the narrower prepared global-memory authority
residual represented by `src/pr36034-1.c` and `src/pr91137.c`.

## Core Rule

Publish prepared global-memory facts only from producer-side facts that prove
global object identity, offset, width, and extent. Do not repair this residual
by changing RV64 emission, test expectations, unsupported policy, allowlists,
timeouts, or accounting.

## Read First

- ideas/open/608_prepared_global_data_authority.md
- todo.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md
- docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md
- src/backend/prealloc/prepared_contract_verifier.cpp
- src/backend/prealloc/object_data.hpp

## Current Targets

- `src/pr36034-1.c`
- `src/pr91137.c`
- Prepared/prealloc global-memory fact publication and verification code that
  owns their first remaining prepared/global authority stop.

## Non-Goals

- Reopening selected object-data authority; the prior Step 4 classified those
  rows as fail-closed `unsupported_but_coherent` or `missing_object_label`.
- Direct global-symbol base-plus-offset authority; the prior Step 3 verified
  that route already publishes its prepared authority.
- Prepared move-bundle authority for `src/990326-1.c`, `src/20060930-2.c`, or
  `src/pr64756.c`.
- Same-module call ABI for `src/20041218-1.c`.
- RV64 global symbol emission, relocation records, global load/store lowering,
  runtime/link behavior, expectations, unsupported markers, allowlists,
  timeouts, or accounting.

## Working Model

The completed runbook moved representative prepared global-memory rows such as
`src/strlen-7.c`, `src/20000703-1.c`, `src/pr58662.c`, `src/20140212-1.c`,
`src/20190228-1.c`, `src/pr57861.c`, and `src/pr58431.c` through the previous
authority stop. It also verified that direct global-symbol base-plus-offset
authority already publishes for representatives such as `src/pr79737-2.c`,
`src/pr82387.c`, `src/pr68624.c`, and `src/pr57568.c`.

The remaining in-scope evidence is therefore narrow: `src/pr36034-1.c` and
`src/pr91137.c` still represent prepared global-memory facts, not selected
object-data authority and not lifecycle close readiness.

## Execution Rules

- Start by re-reading the current diagnostics for both residual cases before
  editing implementation code.
- Keep the packet semantic: either publish a general producer-side prepared
  global-memory fact rule, or prove the residual belongs to a different
  explicit owner.
- Preserve fail-closed diagnostics for missing initializer facts, ambiguous
  object identity, unknown extents, unsupported widths, non-global storage,
  and unsupported sections.
- Do not add testcase-name checks or case-shaped shortcuts for `pr36034-1.c`
  or `pr91137.c`.
- Prove code slices with the supervisor-selected command. The current expected
  narrow proof is:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Ordered Steps

### Step 1: Refresh residual diagnostics

Goal: identify the exact current prepared/global authority stop for
`src/pr36034-1.c` and `src/pr91137.c`.

Primary targets:
- current backend case logs or prepared dumps for both residual cases
- prepared global-memory fact publication code
- `src/backend/prealloc/prepared_contract_verifier.cpp`

Actions:
- Re-run or inspect the current diagnostics for both residual cases.
- Record the missing producer fact for each case: object identity, offset,
  width, extent, initializer fact, section support, or another concrete
  prepared-memory prerequisite.
- Confirm they are still not selected object-data residuals and not RV64
  consumer failures.
- Choose the first implementation packet only after both cases have an owner
  and missing-fact classification.

Completion check:
- `todo.md` records both residual diagnostics, the confirmed first owner, the
  chosen implementation packet, and the exact proof command for Step 2.

### Step 2: Repair the shared prepared global-memory fact rule

Goal: publish the missing supported prepared global-memory fact only when the
producer layer proves the required identity, offset, width, and extent.

Primary targets:
- prepared/prealloc global-memory fact publication code identified in Step 1
- prepared contract verification for global-memory facts
- focused backend rows for `src/pr36034-1.c` and `src/pr91137.c`

Actions:
- Implement the smallest general rule that covers the confirmed residual
  family.
- Reuse existing prepared-fact structures and verifier expectations.
- Preserve unsupported or ambiguous cases as explicit fail-closed diagnostics.
- Keep RV64 lowering and object emission out of the patch.

Completion check:
- At least one residual case moves beyond the prior prepared global-memory
  authority stop, or both are proven to belong to a named downstream owner
  without weakening contracts.

### Step 3: Prove residual handoff and close readiness

Goal: validate the narrowed residual route and decide whether idea `608` can
enter close review or needs another lifecycle split.

Primary targets:
- residual proof rows for `src/pr36034-1.c` and `src/pr91137.c`
- focused backend subset selected by the supervisor
- `todo.md`

Actions:
- Re-run the focused build and selected proof subset after the implementation
  packet.
- Record movement for both residual cases and their next owner, if any.
- Confirm no selected object-data, direct base-plus-offset, move-bundle, ABI,
  RV64 consumer, or unsupported-policy work was absorbed into this runbook.
- State in `todo.md` whether idea `608` is ready for close review.

Completion check:
- `todo.md` contains the residual movement summary, proof command, proof
  result, remaining owner split if any, and close-readiness recommendation.
