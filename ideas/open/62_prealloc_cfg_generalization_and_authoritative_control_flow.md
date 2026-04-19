# Prealloc CFG Generalization And Authoritative Control-Flow Facts

Status: Open
Created: 2026-04-19
Last-Updated: 2026-04-19
Depends-On:
- idea 64 shared text identity and semantic name table refactor
Blocks:
- idea 63 complete phi legalization and parallel-copy resolution

## Intent

The current `src/backend/prealloc/legalize.cpp` control-flow handling is
useful for the prepared x86 pipeline, but it is still bootstrap-shaped. It
recovers branch and join meaning from a narrow set of patterns and emits
authoritative prepared facts only for the cases that the current consumers
already understand.

This idea makes shared prepare own a general CFG/control-flow description that
downstream consumers can trust without reconstructing semantics from local CFG
shape or ad hoc branch/join pattern matching, and it does so on top of the
typed symbolic identity layer from idea 64 rather than raw strings.

## Problem

Current prepare control-flow ownership is incomplete in three ways:

- branch and join ownership is still partially recovered from shape-sensitive
  helpers instead of a fully analyzed CFG model
- `PreparedJoinTransfer` annotation can become ambiguous and then remains
  partially unowned instead of being driven from authoritative graph analysis
- specialized consumers still rely on the current prepared facts being derived
  from select, short-circuit, or loop-carry shapes that happen to match the
  bootstrap implementation

Even where the route wants stable control-flow identity, symbolic keys are
still trending toward string-shaped contracts. That should be corrected by idea
64 before this idea freezes a public CFG/control-flow surface.

That makes the route harder to extend, harder to reason about, and too easy to
overfit around current test shapes.

## Goal

Build an authoritative shared-prepare control-flow model for BIR functions so
consumers take branch, join, and continuation semantics from prepared data
instead of re-deriving them from CFG shape.

## Core Rule

Do not widen the route by adding more testcase-shaped CFG matchers. New work
must improve general CFG ownership and consumer contracts, not just patch one
branch/join shape.

## Primary Surfaces

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- consumers under `src/backend/mir/x86/codegen/`
- any shared prepare helper extracted to keep control-flow analysis coherent

## Desired End State

- shared prepare owns authoritative predecessor/successor and branch-condition
  facts for each function
- join ownership does not depend on narrow post-hoc ambiguity heuristics
- short-circuit and materialized-select consumers read the same authoritative
  control-flow facts rather than bespoke recovered context
- downstream consumers stop reconstructing branch/join semantics from raw CFG
  shape when prepared control-flow data exists

## Concrete Direction

Suggested evolution in `src/backend/prealloc/prealloc.hpp` after idea 64:

```cpp
struct PreparedBlockControlFlow {
  BlockLabelId label;
  std::vector<BlockLabelId> predecessors;
  std::vector<BlockLabelId> successors;
  std::optional<PreparedBranchCondition> branch_condition;
};

struct PreparedControlFlowFunction {
  FunctionNameId function_name;
  std::vector<PreparedBlockControlFlow> blocks;
  std::vector<PreparedJoinTransfer> join_transfers;
};
```

The exact storage shape may differ, but the prepared form needs a stable,
consumer-oriented CFG contract instead of forcing each consumer to infer graph
facts independently.

## Acceptance Shape

This idea is satisfied when shared prepare publishes authoritative CFG and
branch/join facts that cover ordinary branch ownership, join ownership, and
short-circuit continuation queries without requiring consumer-local shape
reconstruction.

## Non-Goals

- full phi destruction mechanics beyond what is needed to publish truthful CFG
  ownership
- generic scalar instruction selection
- x86-local emitter rewrites unrelated to prepared control-flow consumption
- testcase-specific branch matcher growth
- bypassing idea 64 by publishing new string-keyed public CFG contracts
