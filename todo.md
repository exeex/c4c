Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Local-Memory Producer Boundary

# Current Packet

## Just Finished

Step 1 - Locate The Local-Memory Producer Boundary completed as an evidence-only
packet. Added
`docs/rv64_gcc_torture_post_contract/bir_local_memory_producer_boundary.md`
with the concrete boundary decision: local-memory admission is owned by
`BirFunctionLowerer::lower_scalar_or_local_memory_inst(...)` in
`src/backend/bir/lir_to_bir/memory/coordinator.cpp` plus the family producers
`lower_local_memory_alloca_inst(...)`, `lower_memory_gep_inst(...)`,
`lower_memory_store_inst(...)`, and `lower_memory_load_inst(...)` under
`src/backend/bir/lir_to_bir/memory/`, not by prepared/RV64 consumers.

## Suggested Next

Create focused BIR producer tests for the identified boundary before semantic
repairs: load/store `MemoryAddress` plus provenance facts, GEP local array/path
producer-coordinate records, alloca local-slot/source-object records, and
scalar/local-memory bridge cases. Then prove with the five representative RV64
seeds after BIR-side facts are covered.

## Watchouts

- The local-memory row family remains `264` exact semantic rows: `79` load,
  `62` GEP, `58` store, `49` scalar/local-memory, and `16` alloca.
- Representative rows inspected: `src/20000314-1.c`, `src/20000717-4.c`,
  `src/20001026-1.c`, `src/20000519-1.c`, and `src/20050604-1.c`.
- Existing BIR/RV64 tests guard the consumer boundary: prepared/RV64 must reject
  missing or incomplete memory facts and must not infer loads from `LoadLocal`
  shape or pointer decorations.
- Keep call metadata, runtime/intrinsic, scalar/signature/control, and
  bootstrap/global data-shape work separate. Do not weaken semantic admission
  checks, expectations, unsupported markers, allowlists, runtime comparison
  behavior, or downstream consumer fail-closed checks.

## Proof

Evidence-only packet; no compile proof required by delegation and
`test_after.log` was not modified. Inspected existing row docs, representative
logs, `src/backend/bir/lir_to_bir.cpp`,
`src/backend/bir/lir_to_bir/memory/`, `tests/backend/bir/`, and
`c4c-clang-tool-ccdb` symbol/signature output. The direct `function-callees`
queries for `BirFunctionLowerer` member methods did not resolve unqualified
member names in those translation units; the boundary decision is recorded from
AST-backed symbol/signature inventory plus targeted source ranges. No
implementation files, tests, expectation files, unsupported markers,
allowlists, runtime comparison behavior, `test_before.log`, or `test_after.log`
were edited.
