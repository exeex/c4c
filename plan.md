# Select Carrier Alias Metadata Plan

Status: Active
Source Idea: ideas/open/464_select_carrier_alias_metadata.md
Activated From: ideas/closed/463_select_edge_ule_source_producer_rematerialization.md

## Purpose

Publish prepared metadata for duplicate select carrier aliases so later RV64
source-producer rematerialization can consume explicit alias authority instead
of raw select shape.

## Goal

Make prepared control-flow/publication metadata prove when `%t50.phi.sel0`
and `%t50.phi.sel1` are carrier-only aliases of final join-transfer result
`%t50` for the `%t46 = bir.ule ptr %t42, %t45` select-edge source producer.

## Core Rule

Do not infer duplicate-carrier authority from `.phi.sel` names, raw select
shape, value ids, block labels, function names, testcase names, or dump order.
The result must be producer/prepared metadata that a later RV64 consumer can
query directly.

## Read First

- ideas/open/464_select_carrier_alias_metadata.md
- ideas/closed/463_select_edge_ule_source_producer_rematerialization.md
- build/agent_state/463_step3_select_edge_ule_source_producer_route/route.md
- build/agent_state/463_step4_residual_disposition/disposition.md
- src/backend/prealloc/control_flow.hpp
- src/backend/prealloc/prepared_object_traversal.hpp
- src/backend/prealloc/prepared_object_traversal.cpp
- src/backend/prealloc/select_chain_lookups.hpp
- src/backend/prealloc/select_chain_lookups.cpp
- src/backend/prealloc/publication_plans.hpp
- src/backend/prealloc/publication_plans.cpp

## Current Target

- final join-transfer result: `%t50`
- duplicate carrier aliases: `%t50.phi.sel0` / `%t50.phi.sel1`
- selected edge source: `%t46`
- source producer: `%t46 = bir.ule ptr %t42, %t45`
- edge: `logic.rhs.end.40 -> logic.end.41`
- required fact: both carrier values are carrier-only aliases of the same
  `%t50` join-transfer result and do not represent extra non-carrier uses of
  `%t46`

## Non-Goals

- RV64 ULE rematerialization or target consumer changes before metadata
  exists.
- Plain `%t46 -> %t50` copies or same-register no-ops.
- Alias inference from `.phi.sel` names, raw select shape, value ids, block
  labels, function names, testcase names, or dump order.
- Generic stack-to-register, register-to-register, or all-purpose move-bundle
  support.
- Consuming `load_from_stack_slot missing_stack_freshness`.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, runtime-comparison changes, or baseline/log churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

The missing prerequisite is a producer-owned alias fact, not an RV64 lowering
rule. Prepared control-flow/publication metadata should connect duplicate
select carriers to one join-transfer result and prove source-producer use
closure before any target route consumes the relationship.

## Execution Rules

- Step 1 is audit/classification only; do not edit implementation there.
- Any implementation must publish or expose explicit prepared metadata, not
  raw-name or raw-shape inference.
- Preserve fail-closed behavior for mismatched final result, wrong source
  value, wrong edge, missing edge publication, missing source-producer fact,
  raw-name-only carrier shape, and extra non-carrier uses.
- Classification-only proof:

```sh
git diff --check
```

- Code/test proof, if a metadata packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Duplicate Carrier Alias Metadata Gap

Inspect the prepared control-flow, traversal, select-chain lookup, and
publication-plan records for the `%t50.phi.sel0` / `%t50.phi.sel1` carrier
shape. Record which facts already exist, which facts are missing, and where a
durable carrier-alias fact should be produced or exposed. Completion means
`todo.md` contains an audit table and identifies the first metadata packet or
an exact blocker.

### Step 2: Define Carrier-Alias Authority Contract

Specify the accepted carrier-alias fact: function, edge, join transfer, source
value, destination value, carrier values, source producer, and use-closure
requirements. Completion means `todo.md` records positive and negative
contract cases and names target files/tests if implementation is justified.

### Step 3: Implement Or Route Carrier-Alias Metadata Packet

If Step 2 finds a bounded metadata packet, implement only that packet with
focused prepared/BIR coverage. If no implementation is justified, record the
precise blocker and route it to a separate source idea. Completion means proof
passes for the selected packet or lifecycle state records the split/blocker.

### Step 4: Residual Disposition And Close Readiness

Re-check the metadata prerequisite and classify any remaining blocker.
Completion means the source idea closes, remains active with one exact
in-scope metadata packet, or splits a separate durable follow-up.
