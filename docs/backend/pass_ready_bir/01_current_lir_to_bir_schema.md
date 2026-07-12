# Current LIR-to-BIR Schema

This is a current-code inventory, not a target design. Facts below are based on
the public schema in `src/backend/bir/bir.hpp`, the adapter in
`src/backend/bir/lir_to_bir/`, and its direct BIR consumers. The AST-backed
inventory (`c4c-clang-tool[-ccdb] list-symbols`, `function-signatures`, and
focused definition queries) was supplemented with source inspection where a
field's meaning depends on comments or construction order.

## Construction and ownership

`try_lower_to_bir_with_options` analyzes and calls `lower_module`
(`src/backend/bir/lir_to_bir.cpp:595`; `lir_to_bir/module.cpp:1808`). The latter
copies target/name/type context, lowers globals and string-pool entries, emits
extern declarations, then lowers definitions through `BirFunctionLowerer::lower`
(`module.cpp:1811`, `1861`, `1870`, `1894`, `2010`, `2023`). A function is built
from parameters/ABI and local slots, then `lower_block` emits phi instructions,
ordinary instructions, pending copies, and the terminator; switch lowering may
also append synthetic blocks (`module.cpp:1326`, `1484`, `1503`, `1637`, `1710`).
Only after a complete function is returned are block and slot spellings interned
into module-owned tables (`intern_known_block_labels`,
`intern_known_local_slots`, `module.cpp:928`, `968`, `2053`).

Ownership is nested `Module -> vector<Function> -> vector<Block> -> vector<Inst>`
plus an inline `Terminator`. `Module` also owns globals, structured spelling
context, string constants, and `NameTables`; `Function` owns parameters, slots,
many observational/provenance vectors, and atomic records (`bir.hpp:2847-2900`).
No arena, stable Function/Block/Inst handle, use-list, parent link, mutation API,
or analysis cache exists.

## Emitted schema, field by field

| Owner/family | Complete field inventory | Construction / authority |
|---|---|---|
| `Module` | `target_triple`, `data_layout`, `structured_types`, `globals`, `string_constants`, `functions`, `names` | Written by `lower_module`; vectors own objects. `NameTables::{texts,link_names,block_labels,slot_names}` owns the ID tables and reattaches dependent tables after copy/move (`bir.hpp:28-77`). |
| `Global` | display `name`; semantic `link_name_id`; `type`; extern/TLS/constant flags; scalar/integer-array authority flags and element shape; size/alignment; scalar `initializer`; compatibility initializer spelling and semantic `initializer_symbol_name_id`; flattened `initializer_elements`; relocation slots `(byte_offset,size_bytes,target)`; address materialization policy | `lower_minimal_global` / `lower_string_constant_global`, then `resolve_pointer_initializer_offsets` and module-level ID resolution (`module.cpp:1870-1930`; schema `bir.hpp:665-696`). Link IDs are authority when valid; names remain compatibility/display. |
| Structured/string families | `StringConstant{name,name_id,bytes}`; `StructuredTypeFieldSpelling{type_spelling}`; declarations `{name,name_id,fields}`; context `{declarations}` | Derived from LIR string/struct tables by `build_bir_structured_type_spelling_context` (`types.cpp:309`; `module.cpp:1861`). These are spelling-oriented side data, not resolved canonical types (`bir.hpp:698-727`). |
| `Function` scalar/signature | display `name`, semantic `link_name_id`, return type/size/alignment/ABI, calling convention, variadic/declaration flags, `formal_pointer_authority`, address policy | Declaration helpers and `BirFunctionLowerer::lower`; link ID is authoritative when present (`module.cpp:1745-1805`; `bir.hpp:2847-2890`). |
| `Param` / `LocalSlot` | parameter type/name/size/alignment/ABI, varargs/sret/byval flags; slot display name, semantic `slot_id`, type/size/alignment/storage kind/address-taken/byval flags, optional phi observation | ABI lowering (`call_abi.cpp:688-888`), alloca/aggregate/call lowering, followed by slot interning. Slot ID is authority when valid (`bir.hpp:614-644`). |
| Function side tables | local-array source objects, address derivations, element paths, selected proof-edge paths, endpoint bridges, ordered effect streams, interval effects, range proofs, proof facts, checker inputs, local-address provenances, semantic GEPs, scalar-local-load observations; global-static GEP authorities and semantic GEPs; atomic operations | Produced across `lir_to_bir/memory/{provenance,addressing,local_gep,local_slots,coordinator,intrinsics}.cpp` and module lowering. These duplicate relationships to blocks/instructions/values through labels, names, IDs, and indices rather than owning IR nodes. `AtomicOperation` specifically stores block spelling/ID and `inst_index` plus operation/type/width/operands/order/result policy (`bir.hpp:2801-2830`). |
| `Block` | display `label`, semantic `label_id`, `insts`, inline `terminator` | `lower_block`; labels interned after construction (`module.cpp:1710-1732`, `928-966`). Label ID is semantic when valid; spelling is fallback/display (`bir.hpp:2792`). |
| `Value` | immediate/named kind, type, signed immediate, raw immediate bits, optional f128 payload, display SSA-like name, optional semantic pointer-symbol link ID | Created by `Value::immediate_*`, `named`, and `named_symbol_pointer`; equality compares every field (`bir.hpp:446-496`). Ordinary SSA-like named values have no `ValueId`; their names and structural equality drive producer searches. |
| `BinaryInst` | opcode, result, operand type, lhs, rhs | Scalar/aggregate/synthetic-switch lowering (`scalar.cpp:598`, `module.cpp:1438`, `1681`). |
| `SelectInst` | predicate, result, compare type, lhs/rhs, true/false values | Canonical select-function path (`module.cpp:1207-1220`) and scalar lowering. |
| `CastInst` | opcode, result, operand | Scalar conversion lowering (`scalar.cpp:688`, `783`, `868`). |
| `PhiInst` | result, incomings; each incoming has compatibility label, value, semantic label ID | `lower_block_phi_insts` (`module.cpp:1326-1356`). |
| `CallInst` | optional result and result lanes; callee spelling/link ID/optional indirect value; args, arg types, source relationships and ABI; structured/scalar return spellings/type/result ABI; va-arg payload/HFA facts; calling convention and indirect/variadic/noreturn flags; inline-asm/intrinsic metadata; sret slot spelling/ID | `BirFunctionLowerer` call paths (`calling.cpp:1929-2423`, `2460-2534`). This is both semantic instruction data and prepared-policy metadata (ABI, source selection, HFA, inline asm). Direct link ID and slot ID are authoritative when valid. |
| Memory instructions | load-local: result, slot spelling/ID, offset/alignment/address; load-global: result, global spelling/link ID, offset/alignment/address; stores have the same target fields plus value | Aggregate and memory coordinator lowering (`aggregate.cpp:447-624`; `memory/coordinator.cpp`). IDs are authority when valid. `MemoryAddress` additionally carries base kind, base value, slot/global spellings and IDs, displacement, index/scale, address space, and provenance facts (`bir.hpp:815-847`). |
| `Inst` | variant of exactly `BinaryInst`, `SelectInst`, `CastInst`, `PhiInst`, `CallInst`, `LoadLocalInst`, `LoadGlobalInst`, `StoreGlobalInst`, `StoreLocalInst` | Appended directly to block vectors throughout lowering (`bir.hpp:1270-1279`). There is no instruction identity beyond address plus vector index and produced `Value`. |
| `Terminator` | kind; optional return value and return lanes; condition; branch target spelling/ID; true and false spelling/IDs | Built from return/branch/conditional-branch wrappers by `lower_block_terminator` (`module.cpp:1503-1706`). It duplicates all variant fields in one struct (`bir.hpp:2734-2789`). IDs are authoritative when valid. |

The long local-array records are one emitted family each, not hidden IR nodes:
their exact declarations and fields are in `bir_local_array_semantic_gep.hpp`,
`lir_to_bir/memory/memory_types.hpp`, and `bir.hpp:788-814`. Likewise route
1--8 record/index structs in `bir.hpp:1404-2729` are *consumer-built views*, not
module-owned emitted state; this distinction matters because their pointers and
indices become stale.

## Identity conversions and authority

| Identity form | Conversion and authority |
|---|---|
| Link-visible symbol | LIR `LinkNameId` is imported into `Module::names`; globals/functions/calls/global memory/pointer values carry it. `resolve_link_name` yields spelling. Raw-name maps in `lowering.hpp::GlobalTypes` and `ImportedFunctionSymbolIndex` are explicitly no-ID compatibility bridges (`lowering.hpp:40-93`). |
| Block | spelling is interned to `BlockLabelId`; phi and terminator edges receive IDs in `intern_known_block_labels` (`module.cpp:928-966`). Views compare ID when both are valid and otherwise names (`bir_control_flow_view.cpp:9-27`). |
| Slot | spelling is interned to `SlotNameId`, then local memory/address/call-sret references are rewritten (`module.cpp:968-1005`). |
| SSA-like value | `ValueMap` maps LIR spelling to copied `Value`; no stable semantic ID exists. Route records combine `const Value*`, name, optional `ValueNameId`, and equality (`lowering.hpp:29`; `bir.hpp:544-558`). |
| Instruction | address of variant element plus `instruction_index`; `Route1ProducerInstructionIdentity`, atomic records, publication/comparison/call routes all expose indices (`bir.hpp:1404-1412`, `2801-2808`). |
| Function | link ID/name; consumers also retain `const Function*`. There is no `FunctionId`; vector position is implicit iteration/order identity. |
| Prepared lookup | preallocation converts BIR facts into target/prepared keys and plans (`src/backend/prealloc/prepared_lookups.cpp`, `call_plans.cpp`, `publication_plans.cpp`). These are downstream snapshots, not BIR authority. |

## Producers, consumers, and mutation assumptions

The only production route is the LIR adapter above. Direct read consumers are
`bir::validate` (`bir_validate.cpp:723`), printer/render/query, route/view
facades, preallocation, and then MIR/target emitters. Validation checks symbols,
slots, instruction operands and terminators (`bir_validate.cpp:20-723`) but is
not a mutation verifier and does not maintain uses or analyses.

Route 1 indexes same-block producers; route 2 follows select dependencies;
route 3 indexes memory accesses; routes 4/5 model publication and CFG joins;
route 6 call sources/results; route 7 comparisons/branch conditions; route 8
return chains (`bir_route1.cpp` through `bir_route8.cpp`). They scan vectors in
order and store raw object pointers, block labels/IDs, and instruction/record
indices. Examples include `route3_build_memory_access_index`
(`bir_route3_memory.cpp:386`), `route4_build_publication_availability_index`
(`bir_route4_publication.cpp:335`), and explicit pointer-vs-index stale checks
(`bir_route4_publication.cpp:642-648`). Views themselves retain pointers to a
`Block` or `Function` (`bir_control_flow_view.cpp:5-7`). Thus current correctness
assumes construction finishes before consumption; vectors remain ordered and
structurally frozen while a view/index/prepared plan exists; instruction order
is program order; phi instructions remain at block entry; and module/function
iteration remains stable. There are no sanctioned mutation sites after adapter
construction other than module-finalization rewrites before publication
(`module.cpp:316-389`, `928-1005`).

## Required mutation hazards

| Mutation | Concrete current hazard |
|---|---|
| Instruction insertion | Shifts every later `instruction_index`, including atomic and route records; vector reallocation invalidates `Inst*`/`Value*`. `find_selection` interprets its numeric argument directly as `block.insts[number]` (`bir_control_flow_view.cpp:37-45`). |
| Instruction removal | Leaves copied operands without a producer but no use-list diagnoses the dangling semantic use; stored route pointers/indices may now refer to another instruction or be out of range. |
| Instruction reorder | Changes producer-before-use, memory order, comparison fusion, call-source and return-chain meaning. Route queries explicitly use `before_instruction_index` and scan ordered ranges (`bir_route3_memory.cpp:473-486`; `bir_route4_publication.cpp:441`). |
| RAUW | No RAUW exists. Every `Value` is copied into operands, phi incomings, terminators, call-source records, memory/atomic/provenance tables and prepared plans; changing a producer result does not rewrite those copies. Name/equality searches may silently find zero or multiple producers. |
| Block split | Requires moving an instruction suffix, creating/intering a label, choosing a new terminator, redirecting old successors, rewriting phi incoming labels/IDs, and rebuilding all block-pointer/index route state. None is centralized. |
| Edge redirect | CFG authority is the terminator's target fields (`bir_control_flow_view.cpp:16-27`), but each successor phi duplicates predecessor label/ID; routes 4/5 and local-array proof paths duplicate edge facts. Editing only the terminator produces divergent joins/proofs. |
| Function movement | `Module::functions` is a vector. Insert/erase/reorder can invalidate `Function*`, all nested block/inst/value pointers, views, and function-keyed temporary maps (for example `provenance_by_function` at `module.cpp:705`). No parent/ID permits rebinding. |
| Module growth | Appending globals/functions can reallocate both vectors; name tables may grow independently. Link IDs survive table growth, but raw object pointers and prepared snapshots do not, and new symbols are absent from already-built cross-function call/publication plans. |

## Bottom line

Current BIR is a useful immutable publication format with partial semantic IDs,
not a pass-ready mutable IR. Terminators are the closest thing to CFG authority,
but duplicated phi/route/proof state is not automatically synchronized. Stable
link/block/slot IDs protect some references; values, instructions, functions,
ordering, and all pointer/index-based consumer records remain freeze-dependent.
