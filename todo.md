# Current Packet

Status: Active
Source Idea Path: ideas/open/531_bir_memory_provenance_header_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Memory Provenance Boundaries

## Just Finished

Lifecycle activation created this execution state for Step 1 of `plan.md`.

## Suggested Next

Delegate Step 1 as a mapping-only packet: use clang-backed symbol,
type-reference, caller/callee, and include-user queries to map memory
provenance, storage authority, static GEP, dynamic-array, route3, LIR-to-BIR
memory, object emission, and pointer-value provenance consumers. The packet
should recommend the first safe Step 2 declaration boundary or state that idea
531 should park because no behavior-preserving public support header boundary
is available.

## Watchouts

- Keep `bir.hpp` as the compatibility aggregator unless direct include proof is
  available.
- Do not move declarations into private `lir_to_bir/memory/` headers.
- Do not change provenance authority verdicts, storage semantics, enum values,
  record layout, optionality, lookup behavior, or lowering behavior.
- Do not move core `MemoryAddress` usage or edit idea 422 producer behavior
  under this idea.

## Proof

Lifecycle-only activation. No build or tests required.
