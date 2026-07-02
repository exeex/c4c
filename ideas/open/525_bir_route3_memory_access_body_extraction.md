# BIR Route3 Memory-Access Body Extraction

Status: Open
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 6 of 13, after `ideas/open/524_bir_route4_publication_body_extraction.md`
Owning Layer: BIR route3 memory-access analysis
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Extract route3 memory-access record construction and query bodies while keeping
route6 access to the same route3 facts explicit.

## Why This Exists

Route3 is BIR-side indexing over lowered blocks, not LIR-to-BIR memory
lowering. It needs a focused owner without moving public memory authority into
private lowering files.

## In Scope

- Move existing route3 implementation bodies only.
- Add `bir_route3_memory.cpp` and private helper declarations only if needed.
- Preserve public route3 declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map route3 memory symbols, route6
  consumers, and memory type references before moving code.

## Out Of Scope

- Do not move route3 into `src/backend/bir/lir_to_bir/memory/`.
- Do not change `MemoryAddress`, memory provenance, object extent, byte range,
  or storage authority declarations.
- Do not change route6 call-argument publication source policy.
- Do not split public memory model headers in this slice.

## Acceptance Criteria

- Build proof passes.
- Focused memory-access/source proof passes.
- Route6 call-publication tests that consume route3 records pass.
- Memory-access record construction semantics remain identical.

## Reviewer Reject Signals

- Public memory authority records move into private lowering files.
- Route6 loses access to the same route3 facts or duplicates route3 logic.
- Memory access construction semantics change.
- Public header extraction is combined with body movement.
