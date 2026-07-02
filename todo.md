# Current Packet

Status: Active
Source Idea Path: ideas/open/532_bir_local_array_semantic_gep_header_readiness.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Declaration Clusters And Consumers

## Just Finished

Step 1 of `plan.md` audited the local-array proof and semantic-GEP declaration
surface with `c4c-clang-tools` plus narrow source reads.

Mapped declaration clusters in `src/backend/bir/bir.hpp`:

- Local-array source/path cluster: `LocalArrayCarrierStatus`,
  `LocalArrayDerivationKind`, `LocalArrayIndexKind`,
  `LocalArrayLirProducerOperationRole`,
  `LocalArrayLirProducerCoordinateStatus`, `LocalArrayIndexRecord`,
  `LocalArraySourceObjectRecord`, `LocalArrayAddressDerivationRecord`, and
  `LocalArrayElementPathRecord`. Complete prerequisites are `Value`,
  `TypeKind`, `LinkNameId`/`SlotNameId` constants via route prereqs, STL
  containers, and no complete `Function`, `Block`, `Inst`, or lowering types.
- Local-array proof cluster: selected proof edge records, endpoint bridge
  records, ordered effect source stream records, interval effect records,
  range proof inputs/records, proof fact records, checker input records, and
  their inline evaluators/matchers. Complete prerequisites are the source/path
  cluster plus complete `Value`; `evaluate_local_array_interval_effect` has a
  `Function` overload after `Function` storage and must remain outside the
  early declaration header unless Step 2 deliberately keeps that overload in
  `bir.hpp`.
- Local-array provenance and semantic-GEP cluster:
  `LocalArrayLocalAddressProvenance*`, `LocalArraySemanticGep*`, and
  `evaluate_local_array_semantic_gep`. Complete prerequisites are the
  source/path/proof clusters plus complete `Value`; no complete core BIR
  instruction type is needed.
- Static-GEP authority and semantic-GEP cluster:
  `GlobalStaticGepAuthorityStatus`, `GlobalStaticGepDerivationKind`,
  `GlobalStaticGepCoordinateStatus`, `GlobalStaticGepAuthorityRecord`,
  `GlobalStaticSemanticGep*`, and `evaluate_global_static_semantic_gep`.
  Complete prerequisites are complete `Value`, `TypeKind`, `LinkNameId`, and
  memory-provenance declarations from `bir_memory_provenance.hpp`
  (`MemoryLayoutAuthorityKind`, `MemoryRangeVerdict`). This cluster must be
  placed after the current memory-provenance include point.
- Scalar local-load cluster: `LocalArrayScalarLocalLoadStatus`,
  `LocalArrayScalarLocalLoadInputs`, `LocalArrayScalarLocalLoadRecord`,
  `local_array_load_uses_provenance_address`, and
  `evaluate_local_array_scalar_local_load`. The record declaration only needs
  a forward-declared `LoadLocalInst`, but the helper/evaluator dereference
  `LoadLocalInst`, `MemoryAddress`, and `Value`, so the inline behavior must
  stay after the complete `LoadLocalInst`/`MemoryAddress` definitions unless
  Step 2 splits declarations from evaluators.

AST/include evidence:

- `c4c-clang-tool type-refs src/backend/bir/bir.hpp
  LocalArraySemanticGepRecord` found only the inline evaluator and
  `Function::local_array_semantic_geps` storage.
- `c4c-clang-tool type-refs src/backend/bir/bir.hpp
  LocalArrayScalarLocalLoadRecord` found the inline evaluator and
  `Function::local_array_scalar_local_loads` storage; source reads confirm the
  evaluator needs complete `LoadLocalInst` and `MemoryAddress`.
- `c4c-clang-tool type-refs src/backend/bir/bir.hpp
  GlobalStaticSemanticGepRecord` found the inline evaluator and
  `Function::global_static_semantic_geps` storage.
- `c4c-clang-tool-ccdb type-refs` on
  `src/backend/prealloc/publication_plans.cpp` found local semantic-GEP use at
  line 5838, scalar local-load use at line 5941, and static-GEP authority use
  at line 5872.
- `c4c-clang-tool-ccdb type-refs` on
  `src/backend/bir/lir_to_bir/memory/local_gep.cpp` found
  `LocalArrayElementPathRecord` publication at line 183.
- `c4c-clang-tool-ccdb type-refs` on
  `src/backend/bir/lir_to_bir/memory/addressing.cpp` found
  `GlobalStaticGepAuthorityRecord` publication at line 1492.

First safe Step 2 boundary: create an aggregator-only BIR analysis header, for
example `src/backend/bir/bir_local_array_semantic_gep.hpp`, included by
`bir.hpp` after `#include "bir_memory_provenance.hpp"` and before later BIR
instruction/core declarations. Move only the declaration/evaluator cluster that
does not require complete `LoadLocalInst`, `Inst`, `Block`, `Function`, route,
or lowering declarations: local-array source/path records, local-array proof
records and early evaluators/matchers, local-address provenance,
local-array semantic-GEP records/evaluator, and global static-GEP
authority/semantic-GEP records/evaluator. Keep `Function` storage, the
`Function`-taking interval-effect overload, and scalar local-load inline
behavior in `bir.hpp` at their current prerequisite-safe locations for this
first extraction.

## Suggested Next

Delegate Step 2: extract the aggregator-only
`bir_local_array_semantic_gep.hpp` declaration header at the boundary recorded
above, include it from `bir.hpp` after `bir_memory_provenance.hpp`, and run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(lir_to_bir_notes|publication_plan_record|prepare_stack_layout)' > test_after.log 2>&1`

## Watchouts

- Keep this as behavior-preserving declaration-surface cleanup.
- Do not edit lowering behavior, storage semantics, authority policy, tests, or
  expectations to justify a header split.
- Preserve the `bir.hpp` aggregator path unless evidence proves a consumer can
  narrow its include safely.
- Direct include replacement is unsafe in Step 2. `publication_plans.hpp`
  includes `../bir/bir.hpp` while exposing/using complete `bir::Function`,
  `bir::Inst`, and instruction variant types through the prealloc surface;
  `publication_plans.cpp` reaches the target records through that aggregate.
  LIR lowering producers enter through `src/backend/bir/lir_to_bir/lowering.hpp`,
  which includes `../lir_to_bir.hpp` and therefore complete `bir.hpp` plus
  lowering route state. Tests that inspect these records also construct or
  mutate complete `bir::Function`, `bir::Block`, `bir::Inst`, lowering, or
  prepared-module state.
- Include-cycle risk: `bir_memory_provenance.hpp` is not standalone for this
  split because it stores `Value` by value and is currently included from
  `bir.hpp` only after `Value` is complete. The new analysis header should be
  aggregator-only in Step 2 and included after `bir_memory_provenance.hpp`;
  consumers should not include it directly until Step 3 proves a standalone
  prerequisite story.
- Scalar local-load declarations straddle the complete `LoadLocalInst`
  boundary. Moving the record declaration alone is possible, but moving its
  inline evaluator before `LoadLocalInst` would be invalid; keep that cluster
  parked in `bir.hpp` for the first safe split.

## Proof

Mapping-only/no build. Used `c4c-clang-tool` and `c4c-clang-tool-ccdb`
symbol/type-reference queries plus narrow source/include reads; no
`test_after.log` was produced because no compile probe or build was run.
