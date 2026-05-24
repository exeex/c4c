# Subsystem Entropy Reduction Refactor Generator

## Intent

Create an umbrella refactor generator that repeatedly finds low-risk,
behavior-preserving cleanup opportunities and writes focused follow-up ideas
under `ideas/open/`.

This idea does not directly optimize implementation code. Its job is to provide
an unsupervised entropy-reduction protocol: inspect a subsystem, identify where
code grew accidentally while making tests pass, and produce smaller refactor
sub-ideas that can be executed independently without weakening behavior.

## Motivation

Several backend subsystems have grown organically while bringing features up.
That was useful for making tests pass, but it likely left behind duplicated
concepts, historical glue names, oversized headers, thin wrappers, and mixed
responsibility files.

The project's design is not LLVM-shaped, so LLVM should not be used as a direct
layout reference. Instead, use local invariants and behavior-preserving
refactoring rules to reduce entropy over time.

## Entropy-Reduction Rules

Start by classifying code before proposing changes. Use these responsibility
categories:

- data contract: structs, enums, public plan records, stable helper APIs
- phase logic: analysis, planning, normalization, lowering preparation
- adapter logic: translation from stable facts into target-local forms
- emission logic: final target instructions, register spelling, assembly text
- debug/print logic: prepared printers, dumps, diagnostics formatting
- compatibility bridge: legacy fallback, workaround, temporary route glue

A file or helper group is a high-entropy candidate when it mixes several
categories without a durable owner.

## Refactor Slice Types

Generated sub-ideas should usually choose exactly one slice type:

- `Header contraction`: reduce exposed declarations and move private helpers out
  of broad headers.
- `Family merge`: absorb thin `.cpp` files into the family that owns the
  concept.
- `Contract split`: extract stable data contracts from oversized headers.
- `Phase extraction`: split a large implementation file by real phase boundary.
- `Helper absorption`: inline or relocate wrappers into the durable owner.
- `Naming repair`: replace historical workaround names with durable concept
  names.
- `Printer alignment`: align debug/printer files with the data families they
  print.
- `Bridge retirement`: remove compatibility glue that is no longer needed, with
  proof that the newer contract owns the behavior.

Avoid combining multiple slice types in one generated implementation idea unless
the coupling is small and explicitly justified.

## Discovery Inputs

For each audited subsystem, collect:

- line counts with `scripts/count_src_lines.py` or `wc -l`
- largest `.cpp` and `.hpp` files
- broad headers and their include fan-out
- repeated concept names across files
- helpers whose names mention `fallback`, `compat`, `legacy`, `bridge`,
  `temporary`, `workaround`, or old route names
- cross-family includes that may indicate misplaced ownership
- recent forward-migration helpers that now need stable homes
- test/proof commands already used by nearby plans

Use code reading and local tools first. Do not rely on external architecture
references unless the subsystem already has one.

## Candidate Subsystems

Initial candidates include:

- `src/backend/mir/aarch64/codegen`
- `src/backend/prealloc`
- `src/backend/bir`
- `src/codegen/lir`
- `src/frontend/parser`
- `src/frontend/hir`

The plan owner or supervisor may narrow the first run to one subsystem.

## Required Output

The runbook generated from this umbrella idea should produce:

1. A short entropy map for the selected subsystem.
2. A prioritized hotspot list with the reason each hotspot is high entropy.
3. Multiple new `ideas/open/*.md` refactor sub-ideas, each scoped to one
   behavior-preserving cleanup slice.
4. For each generated sub-idea:
   - target files
   - slice type
   - durable owner or responsibility category
   - expected behavior preservation proof
   - explicit reject signals

## Reject Signals

Generated sub-ideas must reject:

- expectation downgrades or unsupported-test conversions
- testcase-shaped shortcuts
- semantic changes hidden inside layout cleanup
- target-specific instruction/register logic moved into generic layers
- giant multi-purpose refactors
- broad renames without proof that the new names represent durable concepts
- refactors that reduce file count while increasing responsibility mixing

## Acceptance

- The umbrella run creates a set of actionable refactor sub-ideas under
  `ideas/open/`.
- Each generated idea is small enough for one focused agent run.
- The generated ideas are ordered by safety and reuse value.
- No implementation code is changed by the umbrella audit unless an explicitly
  tiny mechanical probe is required and separately justified.
- The active plan closes after producing the sub-ideas, or intentionally hands
  off to one activated sub-idea through normal lifecycle rules.
