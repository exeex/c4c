# BIR Local-Memory Semantic Producer Admission

Status: Closed
Type: Producer implementation follow-up
Parent: `ideas/closed/545_bir_semantic_producer_admission_reconstruction.md`
Owning Layer: BIR semantic producer

## Goal

Repair BIR local-memory semantic producer admission for the `264` exact
current `semantic lir_to_bir` rows classified as local-memory facts.

## Why This Exists

The 2026-07-02 evidence reconstruction found the largest exact semantic lane
in local-memory producer topics: `79` load, `62` GEP, `58` store, `49`
scalar/local-memory, and `16` alloca rows. These rows fail before RV64/MIR can
legitimately consume prepared facts, so the producer must publish coherent
semantic memory facts first.

## In Scope

- Inspect `src/backend/bir/lir_to_bir.cpp` and
  `src/backend/bir/lir_to_bir/memory/` for the first missing or malformed
  local-memory facts.
- Add focused BIR tests for load, GEP, store, scalar/local-memory, and alloca
  semantic fact publication.
- Repair semantic producer behavior for those local-memory families.
- Prove progress with focused BIR tests and a narrow RV64 gcc_torture subset
  drawn from representative current rows.

## Out Of Scope

- RV64/MIR-side inference of address, provenance, or memory facts missing from
  BIR.
- Call metadata, runtime/intrinsic, scalar/signature/control, or
  bootstrap/global data-shape repairs.
- Expectation rewrites, unsupported downgrades, allowlist changes, or weaker
  semantic admission checks.
- Broad BIR route rewrites unrelated to current local-memory evidence.

## Acceptance Criteria

- The implementation publishes the semantic facts needed by load, GEP, store,
  scalar/local-memory, and alloca local-memory admission families.
- Focused BIR tests cover the repaired producer behavior.
- A representative RV64 gcc_torture subset includes at least
  `src/20000314-1.c`, `src/20000717-4.c`, `src/20001026-1.c`,
  `src/20000519-1.c`, and `src/20050604-1.c`, or documents a stronger
  current substitute.
- No downstream consumer guesses missing BIR facts.

## Completion Note

Closed on 2026-07-02 after Step 14 reconciliation showed the five
representative RV64 rows had all moved off their local-memory semantic
producer-admission families:

- `src/20000314-1.c`: downstream `unsupported_instruction_fragment`.
- `src/20000717-4.c`: downstream
  `ambiguous_non_parallel_multi_source_stack_destination`.
- `src/20001026-1.c`: downstream `unsupported_instruction_fragment`.
- `src/20000519-1.c`: downstream `unsupported_local_memory_access`.
- `src/20050604-1.c`: downstream `scalar-binop semantic family`.

The remaining failures belong to already separate downstream/open lanes such
as RV64 instruction fragments, prepared local-memory authority, and
scalar/signature/control semantic producer admission. They are intentionally
not absorbed into this local-memory producer idea.

## Reviewer Reject Signals

- Reject named-case shortcuts for only one local-memory testcase shape.
- Reject RV64/MIR lowering that recovers missing address or provenance facts
  from target-specific shapes.
- Reject expectation or unsupported-marker downgrades as producer progress.
- Reject proving only load, GEP, or store while leaving nearby local-memory
  families unexamined.
- Reject helper renames or diagnostic wording changes that retain the same
  semantic admission failure behind a new abstraction name.
