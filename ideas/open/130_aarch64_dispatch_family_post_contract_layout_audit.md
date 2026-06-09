# 130 AArch64 dispatch family post-contract layout audit

## Goal

Re-audit the AArch64 dispatch-family codegen files after the prepared producer,
edge-publication, current-block, calls, memory, wide-value, and i128-shift
contract work has closed.

This idea is analysis-only. It should not move implementation code directly.
Its job is to classify the remaining `dispatch*.cpp` files by durable owner
boundary and produce focused follow-up ideas only for concrete fold-back or
contract gaps.

## Why This Exists

Idea 115 intentionally did not create a mechanical fold-back idea for the
dispatch family because the live residue was still contract/shared-authority
oriented. Idea 116 then closed that contract residue: AArch64 dispatch now
consumes shared prepared facts for edge-copy materializable producer
classification, edge-publication source consistency, current-block join
instruction routing, and direct-global select-chain dependency lookup.

That changes the question. The next question is no longer "which dispatch
logic should move forward to BIR/prealloc?" first. It is:

- which `dispatch_*` files are still useful public hook boundaries;
- which files are now historical splits that should fold back into
  `dispatch.cpp` or a narrower reference-style owner;
- which remaining code is target-local emission glue that should stay in
  AArch64;
- whether any new shared-policy rediscovery remains despite idea 116.

Current dispatch-family files include:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

The reference layout under `ref/claudes-c-compiler/src/backend/arm/codegen`
does not have an equivalent pile of dispatch-specific translation units. If a
current file only exists because earlier work split out temporary helper
surface, it should be turned into a bounded follow-up fold-back idea. If a file
is an intentionally retained API/hook boundary, the closure note should say so
explicitly.

## Required Analysis

For each dispatch-family file, classify its remaining responsibilities as one
or more of:

- `keep-public-hook`: externally used entry point or stable orchestration hook
  that should remain a separate surface for now;
- `fold-back`: target-local helper surface that should move into
  `dispatch.cpp`, `memory.cpp`, `calls.cpp`, `comparison.cpp`, `variadic.cpp`,
  or another natural owner;
- `shared-contract-gap`: target-neutral producer, publication, edge-copy,
  current-block, or select-chain fact still rediscovered by AArch64 codegen;
- `target-local-emission`: AArch64 move/load/store/register hazard or final
  machine instruction emission;
- `local-organization-only`: private helper grouping that may be kept or folded
  only for readability.

The audit must explicitly inspect:

- public declarations in the matching `dispatch*.hpp` files;
- call sites of each dispatch-family public function;
- whether `dispatch_edge_copies.cpp` entry points retained by idea 81 are still
  externally used hooks;
- whether idea 116 left `dispatch_producers.cpp` as a real owner or a small
  consumer wrapper;
- whether `dispatch_publication.cpp` and
  `dispatch_value_materialization.cpp` still need separate translation units;
- whether `dispatch_lookup.cpp` is a useful query boundary or just a thin
  file-local helper split;
- build metadata impact for any proposed fold-back follow-up.

## Expected Output

The closure note must contain:

- a dispatch-family ownership table covering every `dispatch*.cpp` and
  `dispatch*.hpp` file;
- a call-site map for public dispatch-family declarations;
- explicit no-new-idea notes for files that should remain public hooks;
- follow-up ideas for each concrete fold-back or shared-contract gap, with
  bounded files, proof route, and reject signals;
- a note on whether the physical file count can now shrink without changing
  behavior.

If no follow-up is warranted, close with a clear explanation of why the current
dispatch-family split remains useful despite differing from the reference
layout.

## Reject Signals

- Moving code only to reduce line count or file count.
- Merging public hook surfaces while leaving scattered forward declarations,
  hidden cross-file dependencies, or unclear ownership.
- Reintroducing predecessor rescans, BIR-name matching, same-block named-case
  matching, or select-chain special cases.
- Moving AArch64 register hazard policy, instruction spelling, or final
  emission into shared BIR/prealloc code.
- Creating vague ideas such as "clean dispatch" without concrete files and a
  proof route.
- Weakening edge-copy, publication, current-block, select-chain, or backend
  expectations.

## Suggested Proof Route For Follow-Ups

This idea itself is analysis-only. Follow-up implementation ideas should choose
their narrow proof based on touched files. Likely candidates include:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`
- a broader `^backend_` run if a shared prepared query or dispatch contract is
  touched
