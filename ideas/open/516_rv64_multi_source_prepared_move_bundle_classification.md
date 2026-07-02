# RV64 Multi-Source Prepared Move Bundle Classification

Source Parent: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Owning Layer: prepared move-bundle producer and classification contract before RV64 object emission

## Goal

Define and repair the producer/classification contract for prepared move
bundles with multiple register sources targeting one stack value, so RV64
object emission receives either a coherent explicit materialization contract or
a precise producer-owned rejection.

## Why This Exists

The retired runbook for idea 514 established that `src/20010518-1.c` reaches
RV64 object emission as a prepared move bundle with two register-source `i32`
moves targeting the same stack-slot destination value. The bundle has
`authority=none`, `move_count=2`, and `parallel_copy=no`, so object emission
cannot safely materialize it by choosing one source, dropping a move, or
inventing ordering.

This is no longer the single register-source stack-destination diagnostic
handoff from idea 514. It needs a durable owner that decides what the producer
must publish for multi-source prepared move bundles before RV64 object emission
can consume them.

## In Scope

- Reproduce the `src/20010518-1.c` multi-source prepared move bundle and record
  the explicit source homes, destination home, storage facts, scalar types,
  widths, banks, move count, ordering, and rejection site.
- Identify the producer or classifier that creates `authority=none`,
  `move_count=2`, `parallel_copy=no` for multiple register sources targeting
  one stack destination.
- Define the minimal valid contract for this shape: coherent parallel-copy
  authority, explicit sequencing, a producer-owned split into independent
  moves, or a precise rejection before object emission.
- Repair classification or diagnostics so RV64 object emission does not need
  to guess which source owns the destination value.
- Add focused backend or producer-level coverage that proves the accepted
  contract or the producer-owned rejection.

## Out Of Scope

- General parallel-copy scheduling, cycle resolution, or broad move-bundle
  redesign beyond the minimal multi-source-to-one-stack contract.
- Materializing the shape in RV64 object emission without explicit producer
  authority and sequencing.
- Treating `20010518-1.c` as a named special case.
- Inferring move ordering or source ownership from row names, argument order,
  ABI formulas, raw BIR text, or source code variable names.
- Changing unsupported markers, allowlists, runtime comparison, scan
  accounting, or expectations as proof of capability progress.

## Acceptance Criteria

- The multi-source bundle owner is explicit: either the producer publishes a
  coherent contract RV64 can consume, or the producer/classifier rejects it
  before object emission with a precise diagnostic.
- `src/20010518-1.c` no longer reaches RV64 object emission as
  `authority=none`, `move_count=2`, `parallel_copy=no` with ambiguous ownership
  for one stack destination.
- Focused coverage proves the accepted producer contract or the refined
  producer-owned reject path without depending on the row name.
- Validation includes a fresh build, focused representative proof, focused
  coverage for the producer/classifier boundary, and the backend subset chosen
  by the supervisor.

## Reviewer Reject Signals

Reject any route or slice that:

- accepts the multi-source bundle by dropping a move, picking the first or last
  source, or relying on incidental source order
- adds a `20010518-1.c`-shaped shortcut instead of a semantic producer or
  classifier rule
- changes only reason strings, expectations, unsupported markers, allowlists,
  runtime comparison, or scan accounting while the bundle still reaches object
  emission with ambiguous ownership
- claims RV64 object-emission capability progress without explicit producer
  authority, sequencing, and ownership facts
- broadens into a general parallel-copy rewrite before proving this minimal
  multi-source-to-one-stack contract
- renames existing helpers or abstractions while retaining the exact old
  `authority=none`, `move_count=2`, `parallel_copy=no` failure mode
