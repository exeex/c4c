# Complete Phi Legalization And Parallel-Copy Resolution

Status: Open
Created: 2026-04-19
Last-Updated: 2026-04-19
Depends-On:
- idea 64 shared text identity and semantic name table refactor
- idea 62 prealloc CFG generalization and authoritative control-flow facts

## Intent

The current phi handling in `src/backend/prealloc/legalize.cpp` is not a fully
general SSA-destruction route. It can materialize some reducible phi trees into
`bir.select`, and otherwise falls back to slot-backed lowering that is good
enough for the current pipeline but not a complete or principled answer for
arbitrary CFG and phi interaction.

This idea makes phi legalization complete enough for general backend use by
moving from pattern-first recovery to a deliberate phi destruction strategy
with explicit edge transfers, critical-edge handling where needed, and
parallel-copy-safe resolution, all expressed through the typed symbolic
identity/control-flow substrate established by ideas 64 and 62.

## Problem

Current phi lowering is limited by:

- materialization logic that depends on funnel/select-shaped CFG discovery
- fallback lowering that uses local slot stores/loads instead of a fully
  general move/edge-transfer model
- no single authoritative route for resolving phi cycles and parallel copies
  independent of shape
- symbolic identities for blocks, values, and slots should converge on the
  semantic-id substrate from idea 64 rather than freezing more string-shaped
  contracts here

Those constraints make correctness and future backend reuse harder than they
need to be.

## Goal

Provide a general phi legalization path that preserves SSA edge semantics for
arbitrary supported CFG while producing prepared transfer data that downstream
passes can consume without guessing.

## Core Rule

Do not treat one additional phi shape as success. The route must converge on a
general phi destruction model, not a larger pile of named-case materializers.

## Primary Surfaces

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- any extracted shared helpers for edge splitting or parallel-copy planning

## Desired End State

- phi elimination has a general path independent of select/funnel recovery
- edge transfers can represent multiple incoming moves without clobber hazards
- critical edges are handled explicitly when the legalization route requires it
- regalloc and later consumers use authoritative phi-transfer data instead of
  reconstructing phi moves from surviving IR shape

## Concrete Direction

Possible end-state pieces include:

- generalized edge-transfer planning per predecessor/successor edge
- explicit parallel-copy bundles or an equivalent canonical transfer record
- critical-edge splitting only where required by the legality model
- keeping select materialization as an optional optimization instead of the
  correctness path

## Acceptance Shape

This idea is satisfied when phi legalization correctness no longer depends on
select-shaped recovery or slot-only fallback, and downstream move resolution can
consume a deliberate, general transfer model.

## Non-Goals

- changing frontend SSA formation rules
- broad target-specific codegen rewrites unrelated to phi transfer semantics
- folding unrelated branch/join ownership work back into ad hoc x86 helpers
