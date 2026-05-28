# AArch64 Shared Same-Block Materialization Authority Runbook

Status: Active
Source Idea: ideas/open/61_aarch64_shared_same_block_materialization_authority.md

## Purpose

Move same-block scalar and source-producer identity decisions out of AArch64
dispatch code and into shared prepared/query authority, while keeping target
recursive emission local after source facts are known.

## Goal

AArch64 materialization should ask shared/prepared authority for same-block
source facts instead of rediscovering semantic producer identity through local
recursive scans.

## Core Rule

Do not add or preserve AArch64 same-block semantic discovery as the authority.
AArch64 may perform target-local recursive emission only from source facts that
were already established by shared/prepared query ownership.

## Read First

- `ideas/open/61_aarch64_shared_same_block_materialization_authority.md`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- Shared prepared/query owners for same-block value, scalar, load-local, and
  producer facts.

## Current Targets

- Same-block scalar and load-local availability helpers in
  `dispatch_lookup.cpp`.
- Recursive same-block and source discovery in
  `dispatch_value_materialization.cpp`.
- Wrapper consumers in `dispatch_producers.*` that still depend on AArch64
  semantic producer discovery.
- The shared prepared/query owner that should expose replacement same-block
  source facts.

## Non-Goals

- Do not move AArch64 register allocation, scratch handling, instruction
  spelling, or recursive emission sequencing to shared code.
- Do not solve edge-copy dependency fallback, select-chain direct-global
  discovery, or join-copy query migration.
- Do not add testcase-shaped AArch64 shortcuts for one named same-block case.
- Do not rewrite unrelated dispatch-family layout.
- Do not weaken supported-path tests or mark supported paths unsupported to
  hide missing prepared facts.

## Working Model

Same-block materialization currently has AArch64 routes that can discover
semantic producer identity by scanning or recursively inspecting local block
state. This runbook migrates those source facts to shared prepared/query
authority. AArch64 keeps the target-specific work of emitting values once a
prepared same-block fact identifies what source should be materialized.

Missing shared facts must fail closed or produce a durable diagnostic; they
must not silently fall back to a new target-local semantic scan.

## Execution Rules

- Start by inventorying same-block source-discovery call sites before changing
  behavior.
- Separate semantic authority from target emission at every edited helper.
- Prefer a shared prepared/query API that can serve all adjacent same-block
  scalar, load-local, integer-constant, ALU, comparison, and FP consumers.
- Do not introduce raw BIR scans or named-case filters under AArch64 dispatch.
- Keep scratch registers, register spelling, publication ordering, and machine
  instruction emission local to AArch64.
- Record any shared fact gap in `todo.md` when it blocks migration; do not fill
  the gap with an AArch64 fallback.
- For code-changing steps, run at least a fresh build and the focused AArch64
  backend subset chosen by the supervisor. Escalate proof if shared query
  contracts or public helper headers change.

## Ordered Steps

### Step 1: Inventory Same-Block Discovery

Goal: classify every in-scope same-block materialization path by current
authority, source fact needed, and target emission responsibility.

Primary target:
`dispatch_lookup.cpp`, `dispatch_value_materialization.cpp`, and
`dispatch_producers.*`.

Actions:
- Find all same-block scalar, load-local, integer-constant, ALU, comparison,
  and FP materialization helpers that identify producer identity locally.
- Separate helpers that only perform target emission from helpers that discover
  semantic source identity.
- Identify existing shared prepared/query APIs that already expose equivalent
  source facts.
- Identify missing shared prepared/query facts that must be added before
  AArch64 fallback discovery can be removed.
- Record negative cases where missing facts must fail closed or diagnose.

Completion check:
- `todo.md` records the discovery inventory, the shared owner candidate for
  each path, and any blocked fact gaps without implementation edits.

### Step 2: Define Shared Same-Block Source Facts

Goal: expose the same-block source facts AArch64 needs through shared
prepared/query authority.

Primary target:
the shared prepared/query owner identified in Step 1.

Actions:
- Add or extend structured query APIs for same-block scalar, load-local, and
  source-producer facts required by AArch64 materialization.
- Keep query results descriptive enough for AArch64 to emit target code without
  re-scanning for semantic identity.
- Preserve existing prepared authority semantics for missing, ambiguous, or
  unsupported facts.
- Add focused shared-level coverage or assertions where the repository has an
  established test surface for these query facts.

Completion check:
- Shared/prepared APIs can answer the in-scope same-block source questions
  without AArch64 dispatch scans, and missing facts have deliberate
  fail-closed or diagnostic behavior.

### Step 3: Migrate Scalar And Load-Local Availability

Goal: make same-block scalar and load-local availability helpers consume the
shared/prepared facts from Step 2.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`.

Actions:
- Replace local same-block producer or load-local identity decisions with the
  shared prepared/query API.
- Keep AArch64-specific register and home checks local when they are target
  emission details rather than semantic discovery.
- Remove or narrow local helper code that exists only to rediscover source
  identity.
- Preserve behavior for unavailable facts by failing closed or diagnosing
  instead of falling back to local scans.

Completion check:
- Same-block scalar and load-local availability no longer establish semantic
  producer identity through AArch64 scans, and focused coverage exercises both
  available and missing prepared fact cases.

### Step 4: Migrate Recursive Value Materialization

Goal: make recursive same-block materialization use shared source facts while
retaining AArch64 target emission sequencing.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`.

Actions:
- Replace recursive source-discovery decisions with prepared/query source
  facts.
- Keep recursive target emission, scratch handling, register spelling, and
  instruction sequencing inside AArch64.
- Cover integer constants, ALU results, comparison results, and FP results
  that publish values through recursive materialization.
- Remove durable fallback paths that recover semantic source identity locally
  after the shared fact migration.

Completion check:
- Recursive materialization emits only from shared/prepared source facts, and
  no in-scope helper can silently rediscover semantic producer identity in
  AArch64 dispatch.

### Step 5: Update Producer Wrappers And Consumers

Goal: align `dispatch_producers.*` wrapper consumers with the shared
same-block authority.

Primary target:
`src/backend/mir/aarch64/codegen/dispatch_producers.*`.

Actions:
- Replace wrapper consumers that still depend on AArch64 same-block semantic
  discovery with calls to the shared prepared/query facts.
- Remove or narrow wrappers whose only remaining role is local source
  discovery.
- Keep target-local consumers for emission details when they already receive
  source facts from the shared owner.
- Ensure adjacent comparison and FP hooks consume the same shared facts as
  scalar and ALU materialization.

Completion check:
- In-scope producer wrappers no longer own same-block semantic source
  discovery, and their remaining API surface is target-local or removed.

### Step 6: Prove Same-Block Coverage And Fail-Closed Behavior

Goal: validate the migration across adjacent same-block materialization paths
without overfitting one testcase.

Primary target:
focused AArch64 backend tests and any shared prepared/query tests touched by
the migration.

Actions:
- Run a fresh build or compile proof selected by the supervisor.
- Run focused proof for scalar materialization, load-local reuse, integer
  constants, ALU, comparison, and FP recursive value publication.
- Include nearby negative coverage for missing prepared/shared facts.
- Inspect the diff for new AArch64 raw scans, named-case shortcuts, weakened
  expectations, or unsupported downgrades.
- Ask the supervisor to escalate to broader backend or regression guard proof
  if shared query contracts, public headers, or multiple backend buckets were
  changed.

Completion check:
- Build and focused proof are green, `todo.md` records exact commands and log
  names, and the diff contains no AArch64 semantic fallback that replaces the
  old local discovery under a new helper name.
