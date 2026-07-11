# LIR To BIR Adapter Boundary Ordered Follow-Up Plan

Source idea: `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
Plan step: Step 3 - Write The Handoff Documents
Primary inputs:

- `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
- `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`

This is the durable ordered follow-up plan for behavior-preserving
`LIR -> BIR` adapter boundary cleanup. It is not an implementation plan for
this packet and does not create source ideas; Step 4 owns generating
`ideas/open/` files.

## Evidence Set

Durable evidence:

- `docs/lir_bir_adapter_boundary/step1_adapter_boundary_inventory.md`
- `docs/lir_bir_adapter_boundary/step2_responsibility_classification.md`
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`
- `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`

Transient evidence pointers only:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

## Ordering Principles

- Reduce adapter declaration width before moving semantic ownership.
- Hide or split import-local compatibility maps before public BIR, prepared,
  or target consumers are changed.
- Keep behavior-preserving extraction and compile-proof packets ahead of
  semantic repair.
- Preserve BIR core ownership of route schemas and public query surfaces.
- Preserve prepared/prealloc ownership of homes, frame/stack/call/storage
  products, carriers, wrappers, and MIR-facing lookup bundles.
- Route RV64 evidence by first failing layer: exact `semantic lir_to_bir`
  rows are adapter/BIR admission evidence; prepared/module-shape and coherent
  RV64 object diagnostics are downstream evidence; no-diagnostic rows are
  evidence gaps.

## Ordered Follow-Up Families

| Order | Follow-up family | First owning layer | Expected owned surface | Behavior-preserving proof surface | Downstream boundaries not owned |
| ---: | --- | --- | --- | --- | --- |
| 1 | LIR import context extraction | LIR import | `lir_to_bir.hpp`, `lir_adapter_error.hpp`, root `lir_to_bir.cpp`, `analysis.cpp`, `context.cpp`, `module.cpp`, selected import-local state in `lowering.hpp`. | Compile the adapter and the narrow BIR/LIR-to-BIR test target named by the future idea; preserve lowering notes and result envelope behavior. | Do not edit BIR route schemas, prepared module shape, MIR consumers, tests, expectations, unsupported markers, or allowlists. |
| 2 | Private detail header contraction | LIR import | `lowering.hpp` declarations and private helper boundaries that expose split-TU state. | Compile-only or narrow BIR adapter test proof showing declarations moved or hidden without behavior changes. | Do not rename the same responsibility pile behind a new broad abstraction; do not move public BIR records into `lir_to_bir/`. |
| 3 | Structured layout bridge isolation | Structured type/layout bridge | `types.cpp`, `aggregate.cpp`, `memory_helpers.hpp`, structured layout fallback maps, typed operand parsing, `TypeDeclMap`. | Compile proof plus focused existing type/layout adapter tests or dump-equivalence proof selected by the future packet. | Do not change canonical BIR type authority, target aggregate transport lanes, byval ABI placement, or prepared storage layout. |
| 4 | Initializer lowering bridge isolation | Initializer bridge | `globals.cpp`, `global_initializers.cpp`, root string constant rewrite helpers, initializer value materialization, `GlobalTypes`, `FunctionSymbolSet`. | Compile proof plus existing global/initializer adapter coverage selected by the future packet. | Do not change prepared object-data plans, RV64 relocation spelling, emitted global-data layout, tests, expectations, or unsupported markers. |
| 5 | Memory/address provenance import cleanup | Memory/address provenance import | `memory/*.cpp`, `memory_types.hpp`, `memory_helpers.hpp`, local pointer/slot/provenance side tables, pointer-address records, formal pointer provenance publication. | Compile proof plus narrow memory/provenance adapter coverage; escalate only if the future packet edits shared memory/address model files. | Do not move public BIR Route 3 authority into private lowering; do not edit prepared frame/storage policy, target addressing legality, or MIR memory emission. |
| 6 | Call ABI import boundary cleanup | Call ABI import | `call_abi.cpp`, `calling.cpp`, call/return ABI metadata import paths, inline asm and runtime call admission. | Compile proof plus existing call/ABI adapter coverage selected by the future packet. | Do not edit prepared call plans, physical register placement, outgoing stack layout, aggregate lane transport, wrappers, helper protocols, or target emission. |
| 7 | Canonical BIR semantic query follow-up, only if adapter cleanup exposes a true BIR gap | Canonical BIR semantic model | `bir.hpp`, `bir.cpp`, route query implementation, printer, validator, public model records. | Matching before/after route-query or backend subset proof, with prepared surfaces retained as comparison oracles where applicable. | Do not fold raw LIR spelling maps into BIR and do not combine with adapter private-header cleanup. |
| 8 | Prepared/prealloc handoff cleanup, after LIR import contracts are named | Prepared/prealloc handoff | `prealloc.cpp`, `prealloc/module.hpp`, prepared lookup bundles and narrow agreement-gated adapters. | Matching before/after prepared/backend proof selected by a future source idea. | Do not absorb adapter compatibility maps; do not retire `PreparedBirModule` or broad lookup delivery without the agreement gates named in existing fusion docs. |

## Required Step 4 Idea Payload Shape

Step 4 should generate source ideas for at least the first six families unless
it documents a better split. Each idea should name:

- first owning layer
- owned files
- behavior-preserving proof surface
- downstream layers it must not edit
- evidence source docs used
- reviewer reject signals

The required first wave is:

1. `LIR import context extraction`
2. `Private detail header contraction`
3. `Structured layout bridge isolation`
4. `Initializer lowering bridge isolation`
5. `Memory/address provenance import cleanup`
6. `Call ABI import boundary cleanup`

## Cross-Family Guardrails

- `ValueMap`, `GlobalTypes`, `TypeDeclMap`, `FunctionSymbolSet`, local
  slot/pointer maps, structured layout fallback maps, CFG/phi scratch maps,
  and `memory_types.hpp` side tables remain import-local until a follow-up
  proves a narrower adapter contract.
- BIR route records, route indexes, memory address payloads, printer,
  validator, and public query APIs remain canonical BIR semantic model
  authority.
- Prepared plans, homes, frame/stack/call/storage/object products, carriers,
  wrappers, runtime-helper facts, and MIR consumers remain prepared/prealloc
  or target handoff authority.
- RV64, AArch64, and x86 facts must not be moved into canonical BIR or the
  adapter unless the future evidence proves a target-neutral semantic owner.

## Step 3 Handoff

The durable Step 3 handoff consists of:

1. `docs/lir_bir_adapter_boundary/interface_inventory_handoff.md`
2. `docs/lir_bir_adapter_boundary/responsibility_classification_handoff.md`
3. `docs/lir_bir_adapter_boundary/ordered_followup_plan.md`

Together with the Step 1 and Step 2 notes, these documents agree on the same
evidence sources, first-owner vocabulary, transient scan labels, and follow-up
dependency order. Step 4 can now convert the ordered families into source
ideas without reclassifying the boundary.
