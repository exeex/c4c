# LIR To BIR Adapter Boundary Interface Inventory Handoff

Source idea: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
Plan step: Step 3 - Write The Handoff Documents
Primary inventory input:
`docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`

This is the durable Step 3 interface inventory for the current `LIR -> BIR`
adapter boundary. It summarizes the Step 1 inventory without changing source
files, tests, expectations, unsupported markers, allowlists, or tracked build
artifacts.

## Evidence Set

The inventory handoff is grounded in these durable sources:

- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

The following paths are evidence pointers only. They are ignored build outputs
or per-case scan artifacts, not durable lifecycle state:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

## Boundary Inventory

| Boundary region | Files | Durable inventory decision |
| --- | --- | --- |
| Public adapter entry | `src/backend/bir/lir_to_bir.hpp`, `src/backend/bir/lir_adapter_error.hpp`, root `src/backend/bir/lir_to_bir.cpp` | The public contract is the import front door: lowering options, notes, prescan analysis, unsupported/malformed diagnostics, optional BIR result, and throwing convenience entry. |
| Private adapter declaration surface | `src/backend/bir/lir_to_bir/lowering.hpp` | This is the broad split-TU implementation contract for `BirLoweringContext`, module lowering, detail helpers, and `BirFunctionLowerer`. It is not the public BIR model boundary. |
| Private memory/provenance side tables | `src/backend/bir/lir_to_bir/memory/memory_types.hpp` | These records are import-local state for local slots, pointer slots, address ints, dynamic aggregate/pointer arrays, local aggregate slots, pointer addresses, and value materialization. |
| Private memory/layout helper declarations | `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp` | This is the narrow precedent for pure layout/projection helper declarations. It should not absorb stateful lowerer policy or public BIR memory authority. |
| Non-memory adapter implementation | `src/backend/bir/lir_to_bir/{aggregate,analysis,call_abi,calling,cfg,context,global_initializers,globals,module,scalar,types}.cpp` | The implementation is already split by adapter concern: analysis, type/layout import, globals, initializers, aggregate and call ABI, scalar conversion, CFG conversion, calls, context, and module orchestration. |
| Memory adapter implementation | `src/backend/bir/lir_to_bir/memory/{addressing,coordinator,intrinsics,local_gep,local_slots,provenance,value_materialization}.cpp` | The memory implementation imports LIR alloca/load/store/GEP/intrinsic/provenance facts into semantic BIR memory/address records while maintaining route-local side tables. |
| Canonical BIR output model | `src/backend/bir/bir.hpp`, `src/backend/bir/bir.cpp`, printer, validator, route query implementation | BIR owns the semantic module/function/block/instruction/value model, public route records, route indexes, public query surfaces, printer, and validator. |
| Prepared/prealloc handoff | `src/backend/prealloc/prealloc.cpp`, `src/backend/prealloc/module.hpp`, backend route callers | Prepared/prealloc consumes semantic BIR after adapter success and publishes target/layout products such as homes, frame/stack/call/storage/object plans, carriers, wrappers, and MIR-facing lookup bundles. |

## Public Contract Versus Detail Contract

The public contract is intentionally narrow: callers ask the adapter to lower a
`LirModule`, optionally configure import behavior through `BirLoweringOptions`,
receive `BirLoweringResult`, inspect notes and prescan data, or handle
`LirAdapterError`.

The detail contract is broad because `lowering.hpp` exposes private split-TU
declarations and route-local state. The broad detail surface must not be read
as evidence that BIR, prepared/prealloc, or target routes own raw LIR spelling
maps. It is evidence that the adapter needs narrower internal contracts before
later implementation cleanup.

## Import-Local Compatibility State

The following surfaces remain import-local unless a later follow-up proves a
narrower contract:

- `ValueMap`
- `GlobalTypes`
- `TypeDeclMap`
- `FunctionSymbolSet`
- `LocalSlotTypes`
- `LocalPointerSlots`
- `LocalIndirectPointerSlotSet`
- `BackendStructuredLayoutTable`
- `BackendStructuredLayoutEntry`
- `BackendAggregateLayoutLookup`
- `CompareMap`
- `BlockLookup`
- `AggregateValueAliasMap`
- `AggregateParamMap`
- `PhiBlockPlanMap`
- `PendingAggregatePhiCopyMap`
- `PendingScalarPhiProducerMap`
- memory side tables from `memory_types.hpp`

These surfaces are keyed by raw LIR spellings, temporary lowerer handles, or
function-local import state. They should be hidden or split behind private
adapter contracts before any public model or downstream handoff is changed.

## Handoff Rule

Future work should treat `lir_to_bir/` as a private importer that produces
canonical BIR. It may populate BIR-owned semantic records, but it should not
own public route schemas, prepared target/layout products, MIR consumer policy,
or target-specific RV64/AArch64/x86 emission facts.
