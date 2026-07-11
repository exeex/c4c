# LIR To BIR Adapter Boundary Responsibility Classification Handoff

Source idea: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
Plan step: Step 3 - Write The Handoff Documents
Primary classification input:
`docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`

This is the durable Step 3 ownership classification for the current
`LIR -> BIR` adapter boundary. It uses the Step 2 first-owner vocabulary and
keeps transient scan paths as evidence pointers only.

## Evidence Set

Primary evidence:

- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`

Comparison evidence:

- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

Transient evidence pointers:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

## First Owning Layers

| First owning layer | Owned responsibility | Representative current surface | Not owned by this layer |
| --- | --- | --- | --- |
| LIR import | Public adapter entry, diagnostics, options, module prescan, instruction counting, scalar conversion, CFG conversion, raw producer spelling maps, temporary block/phi construction state. | `lir_to_bir.hpp`, `lir_adapter_error.hpp`, `analysis.cpp`, `context.cpp`, `module.cpp`, `scalar.cpp`, `cfg.cpp`, `ValueMap`, `CompareMap`, `BlockLookup`, phi scratch maps. | Canonical BIR route schemas, prepared lookup bundles, target emission policy. |
| Structured type/layout bridge | Legacy type text parsing, type declaration lookup, typed operand parsing, structured layout lookup, aggregate layout fallback, pure layout/projection helpers. | `types.cpp`, `aggregate.cpp`, `lowering.hpp`, `memory_helpers.hpp`, `TypeDeclMap`, `BackendStructuredLayoutTable`, `BackendAggregateLayoutLookup`, aggregate alias maps. | Public BIR type/model authority or target aggregate transport lanes. |
| Initializer bridge | Global declaration import, link-name resolution, string constants, scalar/byte string/array/aggregate/pointer initializer lowering, pointer initializer offsets, known global addresses. | `globals.cpp`, `global_initializers.cpp`, root string constant rewrite helpers, `GlobalTypes`, `FunctionSymbolSet`, initializer value materialization. | Prepared object-data plans, RV64 global relocation spelling, target data emission. |
| Memory/address provenance import | LIR alloca/load/store/GEP/intrinsic facts, local/global memory import, pointer slots, dynamic arrays, pointer-address records, formal pointer provenance imported during lowering. | `memory/*.cpp`, `memory_types.hpp`, `memory_helpers.hpp`, `LocalSlotTypes`, `LocalPointerSlots`, `LocalIndirectPointerSlotSet`, memory side tables. | BIR public memory route authority, prepared frame/storage policy, target addressing legality. |
| Call ABI import | Signature, direct call, call return, byval, vararg, HFA, inline asm, runtime call, and call/intrinsic metadata admitted from LIR into semantic BIR. | `call_abi.cpp`, `calling.cpp`, `module.cpp`, call ABI import helpers and call lowering paths. | Prepared call plans, physical registers, outgoing stack layout, aggregate transport lane policy, wrappers, target helper protocols. |
| Canonical BIR semantic model | Public semantic module/function/block/instruction/value containers, memory address payloads, route records, route indexes, public query APIs, printer, validator. | `bir.hpp`, `bir.cpp`, `bir_printer.cpp`, `bir_validate.cpp`, route records and query helpers. | Raw LIR spelling compatibility maps, prepared homes/layout products, target-specific object routes. |
| Prepared/prealloc handoff | Prepared module assembly, lookup bundles, value homes, frame/dynamic stack/call/storage/object-data plans, carriers, liveness, regalloc, publication policy, MIR-facing products. | `prealloc.cpp`, `prealloc/module.hpp`, prepared docs, backend route handoff callers. | Adapter import compatibility maps or canonical BIR model ownership. |

## Classification Decisions

- Public lowering API and diagnostics are `LIR import`, not general BIR model.
- Target profile inputs are `LIR import` only when they control semantic
  admission. Target emission policy remains prepared/target-owned.
- Legacy type text and structured declarations are `Structured type/layout
  bridge`. Canonical BIR should receive stable type/layout facts, not own raw
  compatibility spelling.
- Global and aggregate initializers are `Initializer bridge`. Their textual
  compatibility should not become public BIR or prepared object-data policy.
- Local/global memory and pointer provenance import are `Memory/address
  provenance import`. BIR Route 3 memory access identity and prepared frame or
  addressing products remain separate owners.
- Call/return ABI metadata admitted from LIR is `Call ABI import`. Concrete
  register placement, outgoing stack layout, byval lane transport, and helper
  protocols remain downstream.
- Route records, route indexes, route facade queries, memory address payloads,
  printer, and validator are `Canonical BIR semantic model`, even when the
  adapter populates their input data.
- `PreparedBirModule`, `PreparedFunctionLookups`, plans, homes, carriers,
  wrappers, and MIR consumers are `Prepared/prealloc handoff`.

## Evidence Alignment

The BIR core cleanup docs support keeping `lir_to_bir/` as private lowering
and keeping public model records central until a separate BIR cleanup proves a
safe split. They also identify `memory_helpers.hpp` as a narrow helper
precedent, not a destination for stateful policy.

The BIR/prealloc fusion docs support moving only target-neutral semantic
relationship facts toward BIR. They reject homes, frame slots, physical
registers, ABI placement, target addressing legality, relocation spelling,
storage hooks, helper protocols, and final instruction records from canonical
BIR.

The RV64 post-contract docs route exact `semantic lir_to_bir` rows to
adapter/BIR admission evidence. Prepared/module-shape diagnostics and coherent
object-lowering diagnostics are downstream boundary evidence. Rows without
explicit diagnostics are evidence gaps and must not be silently assigned to a
named implementation bucket.

## Reject Signals For Future Packets

- Moving raw LIR spelling maps into public BIR, prepared/prealloc, or target
  ownership.
- Treating `PreparedBirModule` or MIR consumers as part of the adapter cleanup
  boundary.
- Treating RV64 scan artifacts under `build/` as canonical lifecycle state.
- Combining LIR import cleanup with BIR route schema changes, prepared
  publication changes, or MIR consumer rewrites in one follow-up.
- Claiming backend capability progress through expectation rewrites,
  unsupported-marker downgrades, allowlist filtering, or named-case shortcuts.
