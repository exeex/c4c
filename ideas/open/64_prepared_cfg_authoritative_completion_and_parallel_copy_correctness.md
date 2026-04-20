# Prepared CFG Authoritative Completion And Parallel-Copy Correctness

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Depends-On:
- idea 62 prealloc CFG generalization and authoritative control-flow facts
- idea 63 complete phi legalization and parallel-copy resolution

## Intent

Recent prealloc work moved prepared control-flow ownership onto `TextId`-backed
records and added `PreparedControlFlowBlock` plus per-edge
`PreparedParallelCopyBundle` data. That is the right direction, but the route
is still not fully authoritative or fully correct.

This idea closes the remaining gap so shared prepare publishes complete
authoritative CFG facts and parallel-copy semantics that downstream consumers
can trust without falling back to heuristic CFG recovery or half-materialized
cycle handling.

## Problem

Three residual issues remain:

- cyclic phi-edge parallel copies are planned with
  `SaveDestinationToTemp` steps, but the temporary-save semantics are not yet
  carried through into the consumer-facing move-resolution path
- `PreparedControlFlowFunction` can still be omitted when a function has only
  block/edge facts and no branch-condition, join-transfer, or parallel-copy
  records, leaving authoritative block-edge metadata unpublished
- join ownership is still partially reconstructed from raw BIR CFG shape rather
  than being derived directly from the prepared control-flow model

These gaps mean the prepared CFG route is improved but not yet complete.

## Goal

Finish the authoritative CFG route so control-flow publication and phi-edge
parallel-copy correctness are both complete enough for backend consumers to
treat prepared control-flow as the sole source of truth.

## Core Rule

Do not patch one failing CFG shape at a time. Fix the missing authoritative
model and the missing cycle-correct transfer semantics instead of growing more
heuristics around the old route.

## Primary Surfaces

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- x86 prepared consumers that currently assume the prepared CFG handoff is
  already authoritative

## Desired End State

- prepared control-flow functions are published whenever authoritative block
  metadata exists
- join ownership is derived from prepared CFG facts rather than raw-BIR
  linear-chain rediscovery
- cyclic parallel-copy bundles have a real end-to-end temporary-save contract
  rather than a plan-only marker
- downstream consumers no longer depend on silent fallback when prepared CFG
  data should be authoritative

## Acceptance Shape

This idea is satisfied when prepared control-flow publication is complete for
covered functions, join ownership is sourced from prepared CFG facts, and
parallel-copy cycles are resolved correctly through the move-resolution path.

## Non-Goals

- reopening string-based naming for prepared metadata
- broad target-specific emitter rewrites unrelated to authoritative CFG
  publication
- adding more select/short-circuit shape matchers as a substitute for general
  prepared CFG ownership
