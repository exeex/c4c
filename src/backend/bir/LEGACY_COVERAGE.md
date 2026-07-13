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
| `bir.cpp`, `bir.hpp`, `bir_private.hpp`, `query.cpp/.hpp`: module/function/block/instruction/value/type identity and traversal | Accepted; reject pointer/name/index identity and duplicate storage schema | Producer/owner: `core/` typed generational IDs, storage, normative order, and immutable views. `verify/` is the validity consumer/gate. |
| `bir_validate.cpp`: structural, type, symbol, CFG, SSA, call, memory, initializer, atomic, and return checks | Accepted; reject boolean/free-form validation and spelling fallback | Producer/owner: the profile rule registries in `verify/`. |
| `lir_to_bir.cpp/.hpp`, `lir_adapter_error.hpp`: import and error transport | Accepted for typed source facts; reject text recovery and partial publication; defer missing structured source carriers | Producer/owner: `lir_to_bir/`. Raw `verify/` is its publication gate; producer gaps remain explicit source failures. |
| `bir_route1.cpp`-`bir_route8.cpp`, `bir_route_facade.cpp`, `bir_route_index*.hpp`: producer, select, memory, publication, call, comparison, and route agreement | Accept the underlying typed semantic facts; reject all Route1-Route8 indices, facades, selected paths, and agreement records | Producer/owner: typed `core/` instructions and def-use. Indexed revision-bound analyses and canonical passes consume those facts and may derive disposable results; they do not preserve a route owner. |
| `bir_producer_view*` | Accept direct definition/producer queries; reject same-block positions, names, pointers, and a persistent producer index | Producer/owner: `core/README.md` exact def-use, `ValueDefinition`, and typed instruction traversal. |
| `bir_select_dependency_view*`, `bir_comparison_view*` | Accept comparison, condition-producer, and select-chain queries; reject selected-path records and persistent dependency snapshots | Producer/owner: `analysis/comparison/README.md`. B2 and later condition consumers use only exact-revision handles. |
| `bir_memory_access_view*` | Accept typed access/effect queries; reject legacy access records, name recovery, and stored access truth | Producer/owner: `analysis/memory_effects/README.md`. Provenance is a read-only dependency/consumer for stronger origin questions, not a second access owner. |
| `bir_memory_provenance.hpp` | Accept conservative object-origin, path, and range queries; reject spelling identity and persistent proof/status records | Producer/owner: `analysis/provenance/README.md`. Canonical memory and C5 address planning are exact-revision consumers. |
| `bir_publication_view*`, `bir_call_boundary_view*`, `bir_return_view*` | Accept semantic value-flow, call-boundary, and return-position queries; reject publication routes, ABI lanes, and source-selection snapshots | Producer/owner: `analysis/publication/README.md`. C3/C4 and D2 consume exact semantic relationships without becoming query producers. |
| `bir_control_flow_view*` | Reject the generic combined view and its relationship snapshots; typed terminators/phi edges, CFG occurrences, conditions, and value flow remain separate facts | Boundary: `core/README.md`, `analysis/cfg/README.md`, `analysis/comparison/README.md`, and `analysis/publication/README.md` retain their disjoint authorities; no combined producer is admitted. |
| `bir_local_array_semantic_gep.hpp` | Accepted semantic address behavior; reject testcase-shaped local-array route | Producer/owner: canonical `passes/memory`. C5 address preparation and later frame mapping consume the normalized address. |
| `bir_printer.cpp`, `bir_render.cpp` | Accepted presentation only; reject rendered identity or compiler feedback | Producer/owner: `diagnostics/` read-only renderers. |
| `prealloc/module.hpp`, `storage.hpp`, `value_locations.hpp`, `names.hpp`, `label_identity.*` | Reject the monolithic prepared graph, physical-home side tables, spelling identity, and label fallback | typed preparation products borrow `CanonicalBir`; `pseudo/` owns its graph revision; abstract homes belong to E2; rendering belongs to `diagnostics/` |
| `prealloc/prepared_lookups.*`, `lookup_agreement.*`, `prepared_object_traversal.*`, `select_chain_lookups.*` | Accept ordinary typed traversal; reject lookup-agreement and duplicate persistent indices | Producer/owner: public `core/` traversal. Product-local and analysis queries remain revision-bound consumers, never a second persistent lookup store. |
| `prealloc/prepared_contract_verifier.*`, `prepared_fact_boundary.hpp` | Accepted invariants; reject one omnibus prepared-state verifier and untyped fact boundary | Producer/owner: `verify/` profile rule registries. Preparation-product checks provide typed inputs to those D3/D4/E4 gates. |
| `prealloc/prepared_printer.*` | Accepted presentation only; reject printer-driven phase truth | Producer/owner: `diagnostics/` over borrowed typed views. |
| `prealloc/legalize.cpp`, `comparison.*`, `control_flow.hpp`, `atomics.*` | Accepted target-independent semantics; reject prepared special cases | Producer/owner: the canonical `passes/` registry and its indexed subordinate pass contracts. D4 and F1 consume the normalized result for target realization. |
| `prealloc/addressing.hpp`, `pointer_value_memory_freshness.hpp` | Accept conservative semantic origin/path queries; reject stored freshness truth, standalone alias authority, and concrete address modes in Canonical BIR | Producer/owner: `analysis/provenance`. C5 address preparation consumes its exact revision-bound result; later concrete mapping cannot turn spelling or an old freshness record into alias proof. |
| `prealloc/calls.hpp`, `call_plans.*`, `formal_publications.*`, `publication_plans.*` | Accepted ABI/call transport behavior; reject publication route records and hidden side-record operands | Producer/owner: shared D2 `passes/call_lowering`. C3/C4 preparation and publication analysis supply verified inputs/read-only availability facts. |
| `prealloc/variadic.hpp`, `variadic_entry_plans.*` | Accepted variadic planning | Producer/owner: C5 `preparation/variadic`. D2 consumes its plan for transport; the later prologue/epilogue boundary only realizes it. |
| `prealloc/inline_asm.*` | Reject as production authority; retain only test/coverage evidence, and reject duplicate interpretation, special carriers, and parsing asm instructions before late assembly | Boundary: C7 `preparation/inline_asm` owns target vocabulary, C9 `regalloc/constraints` binds source descriptions to ordinary values, and the assembler alone interprets template text. |
| `prealloc/intrinsics.*`, `runtime_helpers.hpp`, `i128_runtime_helpers.*`, `f128_runtime_helpers.*` | Accepted intrinsic normalization; reject helper choice in Canonical BIR | Producer/owner: canonical `passes/intrinsics`. C8 helper preparation, D1 pseudo lowering, and D2 calls consume the normalized semantics. |
| `prealloc/target_register_profile.*` | Accepted descriptor behavior; reject concrete-register facts in BIR nodes | Producer/owner: C2 `target_layout` finite abstract pool descriptors. F1 MIR consumes verified assignments for concrete one-to-one mapping only. |
| `prealloc/liveness.*` | Accepted; reject reuse across a revision change | Producer/owner: E1 `analysis/liveness` revision-bound BIR liveness/interference analysis. |
| `prealloc/regalloc.*`, `regalloc_placement_identity.*` | Accepted allocation constraints and behavior; reject legacy physical locations, named-case placement, and MIR allocation fallback | Producer/owner: E2 `regalloc`, the shared pseudo-physical BIR allocator using exact E1 facts. |
| `prealloc/out_of_ssa.cpp` | Accepted; reject residual phi semantics or target-register copies | Producer/owner: D5 `passes/out_of_ssa`, including its allocation-aware copy-resolution closure. |
| `prealloc/decoded_home_storage.*`, `special_carriers.*` | Accept only fixed abstract ABI roles representable as ordinary typed BIR; reject decoded-home mirrors and special value carriers | Producer/owner: D2 `passes/call_lowering`. E2 later assigns ordinary abstract homes; no decoded mirror survives. |
| `prealloc/storage_plans.*` | Accept abstract spill-state planning; reject concrete frame locations or hidden spill transitions | Producer/owner: E3 `regalloc/spill_reload`, which creates abstract spill objects and explicit `Spill`/`Reload` nodes. |
| `prealloc/dynamic_stack.*`, `frame.hpp`, `frame_plan.*` | Accept semantic stack prerequisites; defer concrete layout | Producer/owner: `core/` semantic dynamic-stack operations. C5 address/call facts are planning inputs; concrete frame mapping and prologue/epilogue remain at the indexed `../mir/README.md` boundary. |
| `prealloc/object_data.*` | Defer object bytes and relocations | Boundary: `mir/object`; Canonical BIR retains typed initializer/symbol semantics only. |
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
