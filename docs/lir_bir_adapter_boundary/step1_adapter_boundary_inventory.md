# Step 1 Adapter Boundary Inventory

Source idea: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
Plan step: Step 1 - Inventory The Adapter Boundary

This is a docs-only inventory of the current `LIR -> BIR` adapter boundary.
No implementation files, tests, expectations, unsupported markers, allowlists,
or tracked build artifacts were changed.

## Evidence Sources

Primary adapter sources inspected:

- `src/backend/bir/lir_to_bir.hpp`
- `src/backend/bir/lir_adapter_error.hpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- `src/backend/bir/lir_to_bir.cpp`

Downstream and adjacent boundary sources inspected:

- `src/backend/bir/bir.hpp`
- `src/backend/backend.cpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/module.hpp`
- `tests/backend/bir/CMakeLists.txt`
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`

Transient scan inputs confirmed as evidence pointers only:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`

## Public Boundary Files

| File | Current public role | Boundary observation |
| --- | --- | --- |
| `src/backend/bir/lir_to_bir.hpp` | Public lowering entry for `LirModule` to semantic `bir::Module`. | Exposes `BirLoweringOptions`, `BirLoweringNote`, `BirFunctionPreScan`, `BirModuleAnalysis`, `BirLoweringResult`, `try_lower_to_bir_with_options`, `try_lower_to_bir`, and throwing `lower_to_bir`. |
| `src/backend/bir/lir_adapter_error.hpp` | Public-ish adapter diagnostic exception shape. | Separates unsupported from malformed adapter errors via `LirAdapterError::unsupported`, `malformed`, and `is_unsupported`. |
| `src/backend/bir/bir.hpp` | Canonical semantic BIR model consumed by the adapter output. | Owns `bir::Module`, `Function`, `Block`, `Inst`, `Value`, name tables, type vocabulary, call ABI records, memory-address records, atomics, intrinsics, route records, printer/validator/query declarations. |

The public adapter contract is narrow: callers receive optional semantic BIR,
module prescan data, and notes. The detail contract is much wider because
`lowering.hpp` publishes the split-TU declarations and state types used by the
implementation.

## Detail Boundary Files

| File | Detail role | Boundary observation |
| --- | --- | --- |
| `src/backend/bir/lir_to_bir/lowering.hpp` | Main private declaration surface for the split adapter implementation. | Exposes `BirLoweringContext`, module analysis/lowering orchestration, `lir_to_bir_detail` helper types/functions, and the large stateful `BirFunctionLowerer` method/type surface. |
| `src/backend/bir/lir_to_bir/memory/memory_types.hpp` | Private memory/provenance side-table type surface. | Defines adapter-local records and maps for local/global pointer slots, address ints, local arrays, dynamic aggregate/pointer arrays, local aggregate slots, pointer addresses, and related route-local state. |
| `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp` | Private memory helper declaration surface. | Provides narrower shared helper declarations for layout/projection style memory lowering helpers; existing BIR cleanup docs already identify it as the better destination for pure helper declarations than widening `lowering.hpp`. |

## Implementation Files

Top-level implementation files under `src/backend/bir/lir_to_bir/`:

- `aggregate.cpp`
- `analysis.cpp`
- `call_abi.cpp`
- `calling.cpp`
- `cfg.cpp`
- `context.cpp`
- `global_initializers.cpp`
- `globals.cpp`
- `module.cpp`
- `scalar.cpp`
- `types.cpp`

Memory implementation files under `src/backend/bir/lir_to_bir/memory/`:

- `addressing.cpp`
- `coordinator.cpp`
- `intrinsics.cpp`
- `local_gep.cpp`
- `local_slots.cpp`
- `provenance.cpp`
- `value_materialization.cpp`

Root adapter orchestration:

- `src/backend/bir/lir_to_bir.cpp`

The test build inventory in `tests/backend/bir/CMakeLists.txt` names the same
adapter implementation files for `backend_lir_to_bir_notes_test`, which is a
useful evidence source for future compile-proof surfaces.

## Responsibility Groups

| Group | Current files | First ownership read |
| --- | --- | --- |
| Public lowering API and notes | `lir_to_bir.hpp`, `lir_to_bir.cpp`, `context.cpp`, `analysis.cpp`, `module.cpp` | Adapter-owned import front door and result envelope. |
| Target profile selection and lowering options | `context.cpp`, `module.cpp`, `backend.cpp` callers | Adapter-owned for import behavior; downstream target emission consumes the resulting semantic BIR. |
| Legacy type text and structured layout bridge | `lowering.hpp`, `types.cpp`, `aggregate.cpp`, memory helpers | Adapter-owned compatibility bridge between LIR type spellings/structured declarations and BIR type/layout facts. |
| Global and aggregate initializer bridge | `global_initializers.cpp`, `globals.cpp`, `lir_to_bir.cpp` string constant rewrite helpers | Adapter-owned importer for textual/structured initializers, pointer offsets, string constants, and global address facts. |
| Scalar and CFG lowering | `scalar.cpp`, `cfg.cpp`, `module.cpp` | Adapter-owned semantic producer work over LIR instructions and blocks. |
| Call and return ABI import | `call_abi.cpp`, `calling.cpp`, `module.cpp` ABI pressure adjustments | Adapter-owned import of semantic call/return ABI records from LIR signatures/calls plus `TargetProfile`. Keep physical register/layout policy downstream. |
| Local/global memory and address provenance import | `memory/*.cpp`, `memory_types.hpp`, `memory_helpers.hpp` | Adapter-owned conversion of LIR alloca/load/store/GEP/intrinsic facts into semantic BIR memory/address records and route-local side tables. |
| Canonical BIR semantic model | `bir.hpp`, `bir.cpp`, route files, printer, validator | BIR-owned output model and analysis/query surface, not an adapter implementation detail. |
| Prepared/prealloc handoff | `prealloc.cpp`, `module.hpp`, `backend.cpp` | Downstream consumer of semantic BIR. It publishes prepared facts and MIR-facing plans after the adapter succeeds. |

## Compatibility Maps And Route-Local Tables

`lowering.hpp` exposes several compatibility maps that should be treated as
import-local until a later packet proves a narrower contract:

- `ValueMap`: route-local LIR SSA spelling to lowered BIR value.
- `GlobalTypes`: producer-spelling keyed global information, including
  type text, link-name ids, initializer metadata, pointer initializer offsets,
  runtime element shape, and known global addresses.
- `TypeDeclMap`: legacy LIR type declaration text keyed by producer spelling.
- `FunctionSymbolSet`: authoritative link-name ids plus raw-symbol fallback for
  textual pointer initializer compatibility.
- `LocalSlotTypes`, `LocalPointerSlots`, `LocalIndirectPointerSlotSet`: function-local
  slot/provenance maps keyed by route-local spellings.
- `BackendStructuredLayoutTable`, `BackendStructuredLayoutEntry`,
  `BackendAggregateLayoutLookup`: structured layout plus legacy fallback parity
  bridge state.
- `CompareMap`, `BlockLookup`, `AggregateValueAliasMap`, `PhiBlockPlanMap`,
  `PendingAggregatePhiCopyMap`, `PendingScalarPhiProducerMap`,
  `AggregateParamMap`, and the memory side-table aliases imported from
  `memory_types.hpp`: stateful function-lowering scratch surfaces, not public
  BIR identity.

These maps explain why this umbrella should separate adapter import cleanup
from canonical BIR model cleanup: most keys are raw LIR spellings or temporary
function-local handles.

## Downstream Dependency Evidence

`src/backend/backend.cpp` calls `try_lower_to_bir_with_options` before semantic
BIR dumps, x86/AArch64/RV64 emission, object emission, and prepared handoff
routes. Several target front doors set `preserve_dynamic_alloca = true` before
prealloc/MIR consumption.

`src/backend/prealloc/prealloc.cpp` records that prealloc owns the shared
semantic-BIR to prealloc-BIR route before MIR lowering, then publishes contract
plans such as label identity, frame/dynamic stack plans, call plans, storage
plans, object-data plans, special carriers, atomic operations, intrinsic
carriers, inline asm carriers, and runtime-helper facts.

`src/backend/prealloc/module.hpp` shows `PreparedBirModule` embeds the semantic
`bir::Module`, `TargetProfile`, prepared name/control/value/location/layout
surfaces, call/storage/publication/object data plans, carriers, completed
phases, and notes. This is downstream of adapter success and should not be
folded into the LIR import boundary.

Existing docs provide additional owner evidence:

- `docs/bir_core_cleanup/implementation_inventory.md` and
  `destination_map.md` treat `lir_to_bir/` as a private lowering subsystem and
  warn against moving public BIR memory authority into it.
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md` distinguishes
  semantic BIR relationship candidates from target/layout prepared facts.
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
  classifies exact semantic rows under local-memory, call metadata,
  runtime/intrinsic, scalar/signature/control, and related bootstrap lanes,
  with no exact rows first-owned by prepared/RV64 publication.

## Step 1 Handoff

The concrete adapter boundary for follow-up classification is:

1. Public entry and diagnostics: `lir_to_bir.hpp`, `lir_adapter_error.hpp`.
2. Private adapter declaration surface: `lir_to_bir/lowering.hpp`,
   `lir_to_bir/memory/memory_types.hpp`, `lir_to_bir/memory/memory_helpers.hpp`.
3. Implementation split: root `lir_to_bir.cpp`, top-level `lir_to_bir/*.cpp`,
   and `lir_to_bir/memory/*.cpp`.
4. Output and downstream consumers: `bir.hpp`, `backend.cpp`,
   `prealloc/prealloc.cpp`, `prealloc/module.hpp`.
5. Durable evidence docs: `docs/bir_core_cleanup/`,
   `docs/bir_prealloc_fusion/`, and
   `docs/rv64_gcc_torture_post_contract/`, with `build/agent_state/*` treated
   only as transient scan pointers.

Step 2 should classify the groups above by first owning layer before proposing
any behavior-preserving extraction or follow-up ideas.
