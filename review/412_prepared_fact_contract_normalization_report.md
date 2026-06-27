# Prepared Fact Contract Normalization Report

Date: 2026-06-27
Source idea: `ideas/open/412_prepared_fact_contract_normalization_analysis.md`

## Decision

The concern is confirmed: the highest-risk design debt is not semantic BIR
itself, but the BIR/prealloc/prepared fact contract presented to MIR/RV64 and
other target consumers.

`PreparedBirModule` currently carries many parallel fact families in one
aggregate: value locations, stack layout, addressing, regalloc, frame plans,
call plans, store/call publication plans, variadic plans, storage plans,
wide-value carriers, intrinsic/inline-asm carriers, and runtime helpers
(`src/backend/prealloc/module.hpp:38`). `PreparedFunctionLookups` then exposes a
target-facing bag of call, address, memory, move, value-home, and publication
lookups (`src/backend/prealloc/prepared_lookups.hpp:15`).

That shape has worked for incremental RV64 bring-up, but the 395-411 closure
chain shows repeated producer-boundary repairs. Future progress should normalize
the prepared fact contract before adding more RV64 fixups.

## Architecture Map

- Semantic BIR production owns source semantics, typed values, globals, memory
  provenance, and pre-prepared failures. Idea 410 is the clearest recent
  example: packed bitfield scalar binop lowering was repaired before prepared
  handoff, not in RV64.
- Prealloc/prepared owns ABI homes, stack/frame layout, value homes, memory
  access facts, helper operand homes, call plans, and publication records.
  Ideas 403, 405, 407, 408, and 409 all closed through this layer.
- Target consumers may lower coherent prepared facts to instructions, object
  data, relocations, and runtime sequences. Ideas 406 and 411 are examples of
  target-side work after prepared facts were coherent.
- Diagnostics currently live partly in producer records and partly in consumer
  rejection paths. `publication_plans.hpp` already has missing/incomplete fact
  statuses (`src/backend/prealloc/publication_plans.hpp:26`), but there is not
  yet one verifier gate that rejects malformed prepared contracts before target
  lowering.

## Confirmed Refactor Areas

### 1. Prepared Contract Verifier

Confirmed.

Evidence:
- Missing/incomplete producer states exist as local enums, such as edge
  publication source and memory-access statuses
  (`src/backend/prealloc/publication_plans.hpp:65` and
  `src/backend/prealloc/publication_plans.hpp:88`).
- The same missing-fact classes are still discovered by RV64 object-route
  failures during gcc_torture exploration.
- Ideas 403, 405, 407, 408, and 409 all required recognizing producer-owned
  missing facts before accepting target work.

Plan:
- Add a prepared contract verifier/report pass that runs after publication and
  before target consumers.
- Start in report-only/fail-closed mode for selected fact families.
- Classify diagnostics as producer-missing, producer-incoherent,
  target-unsupported-coherent, or semantic-prepared-handoff.

### 2. Typed Call Argument Contracts

Confirmed.

Evidence:
- `PreparedCallArgumentSourceSelection` uses `kind` plus many optional fields
  covering preservation, frame-slot address/value, address materialization, and
  byval lane routes (`src/backend/prealloc/calls.hpp:53` and
  `src/backend/prealloc/calls.hpp:81`).
- RV64 consumers must repeatedly check legal field combinations before emitting
  a route, for example prior-preserved arguments in
  `src/backend/mir/riscv/codegen/prepared_call_emit.cpp:110`.
- Ideas 403 and 407 show ABI/home publication gaps are producer contracts, not
  consumer inference opportunities.

Plan:
- Convert the source-selection optional bag into typed route payloads.
- Add verifier rules for each route payload before changing target behavior.
- Keep target consumers as exhaustively matched route consumers.

### 3. Value Materialization And Rematerialized Constants

Confirmed.

Evidence:
- `PreparedValueHomeKind` includes `RematerializableImmediate` and
  `PointerBasePlusOffset` (`src/backend/prealloc/value_locations.hpp:19`), with
  payload fields living in `PreparedValueHome`.
- Idea 404 fixed wide rematerialized immediate admission as a prepared producer
  boundary and explicitly rejected arbitrary RV64 BIR constant reconstruction.
- RV64 still contains same-block producer discovery helpers for select/binary
  forms (`src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp:193` and
  `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp:220`). Some of that
  is legitimate target scheduling, but it needs a contract boundary audit.

Plan:
- Define first-class materialization facts for immediates, pointer deltas, and
  select/binary producer chains.
- Verifier rejects named values that require target materialization but lack a
  complete materialization fact.
- Target consumers may schedule or lower only published materialization facts.

### 4. Helper Operand Homes

Confirmed.

Evidence:
- `PreparedVariadicEntryHelperOperandHomes` is a helper-kind struct with
  optional homes/plans for `va_start`, `va_arg`, aggregate `va_arg`, and
  `va_copy` (`src/backend/prealloc/variadic.hpp:306`).
- Missing fact strings are manually appended for every helper operand
  (`src/backend/prealloc/variadic_entry_plans.cpp:570`).
- Idea 408 closed only after producer-side publication added explicit
  `va_start.destination_va_list_address`; RV64 was not allowed to infer it
  from stack/source/BIR shape.

Plan:
- Convert helper operand homes into typed helper contracts.
- Add per-helper completeness checks and precise unsupported diagnostics.
- Keep RV64 helper lowering as a consumer of typed operand contracts.

### 5. Storage Layout And Global Initializers

Confirmed.

Evidence:
- `PreparedAddress` carries base kind, size, alignment, offset, and provenance
  (`src/backend/prealloc/addressing.hpp:95`), while `PreparedMemoryAccess`
  wraps the address per instruction (`src/backend/prealloc/addressing.hpp:237`).
- Idea 405 repaired contradictory local aggregate/union slot extents in the
  producer; idea 406 then handled coherent target-side memory boundaries.
- Idea 409 repaired packed/fp128 global initializer admission before RV64 data
  emission. RV64 global emission still inspects BIR globals directly for
  simple supported storage shapes (`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp:75`).

Plan:
- Normalize storage object facts: slot extent, byte range, alignment, overlay
  and provenance.
- Normalize global storage/initializer facts as bytes/relocations/admission
  outcomes, so object emitters do not reconstruct initializer semantics.
- Add verifier checks before enabling broader object emission.

### 6. Target Consumer Boundary Audit

Confirmed as necessary.

Evidence:
- Some RV64 consumer logic is clean coherent-fact lowering, especially after
  ideas 406 and 411.
- Some RV64 logic searches BIR blocks or prepared module globals to recover
  producer/source information. The select/binary helpers and global-storage
  probes are the main review targets.

Plan:
- Audit RV64 and AArch64 prepared consumers for semantic reconstruction,
  fallback lookup by source spelling, and layout/ABI guessing.
- Route each finding either to a producer contract idea or to a target
  coherent-fact lowering idea.

## Rejected Or Inconclusive Areas

- A wholesale BIR IR rewrite is rejected for now. The evidence points first to
  prepared contract shape, not the core BIR instruction model.
- Terminator lowering from idea 396 did not surface a producer CFG ownership
  gap in the selected representatives.
- Move-bundle target shapes from idea 397 looked like target lowering over
  existing prepared authority after refresh, but move/publication facts should
  still be covered by the verifier and target-boundary audit.

## Migration Order

1. Add verifier taxonomy and report-only hooks for prepared contract families.
2. Audit RV64 and AArch64 target consumers early, using the initial taxonomy
   to identify producer-fact recovery helpers before the typed-contract slices
   choose their exact scope.
3. Convert call-argument source selection to typed contracts.
4. Normalize materialization facts for rematerialized constants and producer
   chains.
5. Normalize helper operand contracts, starting with variadic helpers.
6. Normalize local/global storage and initializer contracts.
7. Continue target-consumer cleanup as the typed contracts land, deleting or
   quarantining fact-recovery helpers once their producer contracts exist.

Each step should prove default CTest stability. RV64 gcc_torture pass count may
temporarily drop during contract normalization because it is not the default
harness guard; use it as directional evidence and representative proof, not as
the non-regression gate.

## Follow-Up Ideas Generated

- `ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md`
- `ideas/open/418_prepared_target_consumer_boundary_audit.md`
- `ideas/open/414_typed_prepared_call_argument_contracts.md`
- `ideas/open/415_prepared_value_materialization_contracts.md`
- `ideas/open/416_prepared_helper_operand_home_contracts.md`
- `ideas/open/417_prepared_storage_layout_and_initializer_contracts.md`
