# Legacy Capability Coverage Ledger

Status: reviewed disposition ledger for the current `src/backend/legacy/`
inventory. This is migration evidence, not permission to compile, restore, or
extend legacy code.

`Accepted` means the semantic behavior has exactly one named new owner.
`Reject` means the old representation or alternate route is deliberately not
retained. `Defer` means the capability belongs to a named later non-BIR owner
or remains an explicit source/implementation gap; no earlier stage may infer
it. A legacy file may supply test vectors and audit evidence after its
authority is rejected.

| Legacy files or capability family | Disposition | Accepted owner or boundary |
|---|---|---|
| `bir.cpp`, `bir.hpp`, `bir_private.hpp`, `query.cpp/.hpp`: module/function/block/instruction/value/type identity and traversal | Accepted; reject pointer/name/index identity and duplicate storage schema | `core/` owns typed generational IDs, storage, normative order, and immutable views; `verify/` owns validity |
| `bir_validate.cpp`: structural, type, symbol, CFG, SSA, call, memory, initializer, atomic, and return checks | Accepted; reject boolean/free-form validation and spelling fallback | profile rule registries in `verify/` |
| `lir_to_bir.cpp/.hpp`, `lir_adapter_error.hpp`: import and error transport | Accepted for typed source facts; reject text recovery and partial publication; defer missing structured source carriers | `lir_to_bir/`, Raw publication in `verify/`, and the source producer for explicit gaps |
| `bir_route1.cpp`-`bir_route8.cpp`, `bir_route_facade.cpp`, `bir_route_index*.hpp`: producer, select, memory, publication, call, comparison, and route agreement | Accepted behavior; reject all Route1-Route8 indices, facades, selected paths, and agreement records | typed core def-use plus `analysis/{cfg,dominance,provenance,publication,liveness}` and relevant canonical passes |
| `bir_{producer,select_dependency,memory_access,publication,comparison,control_flow,call_boundary,return}_view*` and `bir_memory_provenance.hpp` | Accepted queries; reject persistent view/lookup authority and pointer-keyed caches | public core views and revision-bound analyses; calls/returns additionally use preparation call facts and D2 |
| `bir_local_array_semantic_gep.hpp` | Accepted semantic address behavior; reject testcase-shaped local-array route | `passes/memory`, preparation `address`, then MIR frame mapping |
| `bir_printer.cpp`, `bir_render.cpp` | Accepted presentation only; reject rendered identity or compiler feedback | `diagnostics/` read-only renderers |
| `prealloc/module.hpp`, `storage.hpp`, `value_locations.hpp`, `names.hpp`, `label_identity.*` | Reject the monolithic prepared graph, physical-home side tables, spelling identity, and label fallback | typed preparation products borrow `CanonicalBir`; `pseudo/` owns its graph revision; abstract homes belong to E2; rendering belongs to `diagnostics/` |
| `prealloc/prepared_lookups.*`, `lookup_agreement.*`, `prepared_object_traversal.*`, `select_chain_lookups.*` | Accepted queries where still needed; reject lookup-agreement and duplicate persistent indices | typed product keys, core traversal, and revision-bound analyses |
| `prealloc/prepared_contract_verifier.*`, `prepared_fact_boundary.hpp` | Accepted invariants; reject one omnibus prepared-state verifier and untyped fact boundary | owning preparation-product verifiers, D3/D4 Pseudo profiles, and E4 Allocated profile in `verify/` |
| `prealloc/prepared_printer.*` | Accepted presentation only; reject printer-driven phase truth | `diagnostics/` over borrowed typed views |
| `prealloc/legalize.cpp`, `comparison.*`, `control_flow.hpp`, `atomics.*` | Accepted target-independent semantics; reject prepared special cases | canonical `passes/{legalize,scalar,cfg,memory}`; target realization is D4/MIR |
| `prealloc/addressing.hpp`, `pointer_value_memory_freshness.hpp` | Accepted planning/query behavior; reject stored freshness truth and concrete address modes in Canonical BIR | `analysis/{provenance,alias}`, preparation `address`, and later MIR selection |
| `prealloc/calls.hpp`, `call_plans.*`, `formal_publications.*`, `publication_plans.*` | Accepted ABI/call transport behavior; reject publication route records and hidden side-record operands | preparation `abi` then `calls`; shared D2 call lowering; publication analysis for semantic availability |
| `prealloc/variadic.hpp`, `variadic_entry_plans.*` | Accepted | preparation `variadic`, shared D2 transport, MIR prologue/epilogue |
| `prealloc/inline_asm.*` | Accepted only as test/coverage input; reject duplicate interpretation, legacy special carriers, and parsing asm instructions before late assembly | preparation `inline_asm` owns target vocabulary; `regalloc/constraints` alone binds source descriptions to ordinary values; assembler interprets template text |
| `prealloc/intrinsics.*`, `runtime_helpers.hpp`, `i128_runtime_helpers.*`, `f128_runtime_helpers.*` | Accepted; reject helper choice in Canonical BIR | canonical `passes/intrinsics`, preparation `runtime_helpers`, D1 pseudo lowering, shared D2 calls |
| `prealloc/target_register_profile.*` | Accepted descriptor behavior; reject concrete-register facts in BIR nodes | target layout and finite abstract pool descriptors; MIR performs concrete mapping |
| `prealloc/liveness.*` | Accepted; reject reuse across a revision change | E1 revision-bound BIR liveness/interference analysis |
| `prealloc/regalloc.*`, `regalloc_placement_identity.*` | Accepted allocation constraints and behavior; reject legacy physical locations, named-case placement, and MIR allocation fallback | E2 shared pseudo-physical allocator using the exact E1 facts |
| `prealloc/out_of_ssa.cpp` | Accepted; reject residual phi semantics or target-register copies | D5 `passes/out_of_ssa` with explicit edge/copy operations |
| `prealloc/decoded_home_storage.*`, `storage_plans.*`, `special_carriers.*` | Accepted only where representable as ordinary typed roles; reject decoded-home mirrors and special value carriers | D2 fixed abstract roles, E2 assignments, E3 explicit `Spill`/`Reload`, and MIR frame mapping |
| `prealloc/dynamic_stack.*`, `frame.hpp`, `frame_plan.*` | Defer concrete layout; accept semantic stack prerequisites | canonical memory/core semantics, preparation address/call facts, then MIR frame layout and prologue/epilogue |
| `prealloc/object_data.*` | Defer object bytes and relocations | `mir/emission`; Canonical BIR retains typed initializer/symbol semantics only |
| `prealloc/prealloc.cpp/.hpp`, `prealloc/README.md` | Reject the monolithic driver, phase flags, completed-phase strings, and alternate publication route | root ordered pipeline and its named stage owners |

## Coverage gate

The inventory above covers every current file under `src/backend/legacy/`,
including `prealloc/`. Architecture review must fail if a new file or externally
used symbol is not added here with `Accepted`, `Reject`, or `Defer` disposition.
An `Accepted` row requires exactly one producer/owner and may name multiple
read-only consumers. A `Defer` row must remain forbidden at every earlier
profile. A `Reject` row must not be recreated under a new name or hidden in a
cache, renderer, adapter, target hook, or test helper.

Before implementation acceptance, symbol-level review must record exact
input/output fields, failure behavior, target differences, adjacent
same-feature tests, and deletion of duplicated routes. Coverage is incomplete
if proof demonstrates only a named testcase, rewrites an expectation, or
leaves a nearby member of the same feature family unexamined.
