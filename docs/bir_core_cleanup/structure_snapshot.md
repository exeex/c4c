# BIR Core Model Structure Snapshot

Source idea: `ideas/open/518_bir_core_model_cleanup_umbrella.md`
Plan step: Step 1 - Establish Structure Snapshot

This is an analysis-only snapshot. No implementation files were changed.

## File Sizes

Command:

```sh
wc -l src/backend/bir/bir.hpp src/backend/bir/bir.cpp
```

Result:

```text
  6324 src/backend/bir/bir.hpp
  5158 src/backend/bir/bir.cpp
 11482 total
```

## Tool Availability

Command:

```sh
command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb
```

Result:

```text
/home/vscode/.local/bin/c4c-clang-tool
/home/vscode/.local/bin/c4c-clang-tool-ccdb
```

No install fallback was needed. `scripts/build_install_c4c_clang_tools.sh`
was not run.

## AST Query Log

Header queries used the flags entrypoint because headers do not appear directly
in `build/compile_commands.json`:

```sh
c4c-clang-tool list-symbols src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool type-refs src/backend/bir/bir.hpp Value -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool type-refs src/backend/bir/bir.hpp MemoryAddress -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool type-refs src/backend/bir/bir.hpp Inst -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
```

Implementation queries used the compile database entrypoint:

```sh
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/bir.cpp route3_memory_access_record build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/bir.cpp route4_build_publication_availability_index build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/bir.cpp route7_build_comparison_condition_index build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/bir.cpp find_call_argument_publication_source_routing build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/bir/bir.cpp route3_memory_access_record build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/bir/bir.cpp route4_build_publication_availability_index build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/bir/bir.cpp route7_build_comparison_condition_index build/compile_commands.json
c4c-clang-tool-ccdb type-refs /workspaces/c4c/src/backend/bir/bir.cpp Route3MemoryAccessRecord build/compile_commands.json
c4c-clang-tool-ccdb type-refs /workspaces/c4c/src/backend/bir/bir.cpp Route7ComparisonConditionIndex build/compile_commands.json
```

The raw `list-symbols` and `function-signatures` JSON was large, so the
snapshot records aggregated counts and selected dependency probes instead of
embedding full JSON output.

## AST Results Summary

`bir.hpp` symbol inventory:

```text
constructor: 7
enum: 80
function: 200
method: 15
namespace: 3
struct: 132
type_alias: 1
variable: 69
```

`bir.hpp` function signature count: 200.

`bir.cpp` symbol inventory:

```text
function: 157
method: 11
namespace: 15
struct: 1
type_alias: 9
variable: 350
```

`bir.cpp` function signature count: 157.

Selected type-reference probes:

| Query | Result |
| --- | --- |
| `bir.hpp` `Value` | 191 references; first hits at lines 463-470 |
| `bir.hpp` `MemoryAddress` | 9 references; first hits include lines 4066, 4067, 4104, 4306, 4319, 4332, 4343, 4411 |
| `bir.hpp` `Inst` | 19 references; first hits include lines 4481, 4527, 4537, 4539, 4604, 4660, 4737, 4832 |
| `bir.cpp` `Route3MemoryAccessRecord` | references cluster at lines 651-804 with later same-block use around 878, 933, 982 |
| `bir.cpp` `Route7ComparisonConditionIndex` | 23 references; first hits at lines 3774, 3795, 3831, 3858, 3886, 3914, 3916, 3946, 3948 |

Selected caller/callee probes:

| Query | Summary |
| --- | --- |
| Callees of `route3_memory_access_record` | `route3_memory_access_node_kind`, `route3_memory_address`, `route3_memory_access_base_kind`, `route3_result_value`, `route1_source_value_identity`, `route3_stored_value`, plus an assignment/operator hit |
| Callers of `route3_memory_access_record` | `route3_memory_access_result_value_record`, `route3_memory_access_stored_value_record`, `route3_build_memory_access_index`, `route3_same_block_global_load_access_record`, `route3_same_block_load_local_source_record`, `route6_call_argument_publication_source_record` |
| Callees of `route4_build_publication_availability_index` | `route1_build_producer_index`, `route4_current_block_publication_record`, `route4_current_block_publication_value_record`, `route4_block_entry_publication_record`, `route4_block_entry_publication_value_record`, plus `operator bool` hits |
| Callers of `route4_build_publication_availability_index` | Tool returned `ok: false`, error `target not called in this translation unit` |
| Callees of `route7_build_comparison_condition_index` | `route7_comparison_instruction_record`, `route7_branch_condition_record` |
| Callers of `route7_build_comparison_condition_index` | `find_materialized_condition_producer_identity` |
| Callees of `find_call_argument_publication_source_routing` | `find_call_argument_source_relationship`, `call_argument_source_selection_available`, `call_argument_direct_global_select_chain_dependency_available` |

## Raw-Text Fallback Notes

AST queries succeeded for the required inventory and dependency probes. A
narrow raw-text pass was still used to anchor line ranges and headings because
the full JSON output was too large for a readable durable artifact:

```sh
rg -n "^(enum class|struct|inline|std::|bool|const|Route[0-9]|Value|Type|Memory|Module|Function|Block|Inst|void|int|std::optional)" src/backend/bir/bir.hpp | sed -n '1,220p'
rg -n "^(Route[0-9]|Call|Comparison|Fused|Materialized|std::optional|const|bool|void|std::string|Type|Value|Memory|Module|Function|Block|Inst)" src/backend/bir/bir.cpp | sed -n '1,240p'
sed -n '1,80p' src/backend/bir/bir.cpp
sed -n '420,760p' src/backend/bir/bir.hpp
```

This fallback did not replace the AST pass; it only supplied compact line
anchors for the clusters below.

## Initial Symbol And Dependency Clusters

### `bir.hpp`

| Lines | Cluster | Initial observation |
| --- | --- | --- |
| 17-84 | Forward declarations, `NameTables`, type identity | Central model identity surface; likely remains public and central. |
| 84-431 | Core enums and naming helpers | `TypeKind`, atomics, intrinsic families, calling conventions, and ABI classes are public model vocabulary. |
| 439-637 | `Value`, phi/call ABI records, params, local slots | Core IR value and function/block scaffolding starts here. `Value` has heavy fanout and should be moved late, if at all. |
| 637-2373 | Local array provenance and proof records | Large route-specific analysis surface in the public model header; likely a candidate family for later classification. |
| 3394-3806 | Globals, strings, structured type spelling, memory provenance, static GEP authority | Mixes core global/module data with memory authority analysis records. Needs careful separation between public model contract and route-local proof facts. |
| 3933-4343 | Instruction variants and call argument source routing records | Core instruction model plus route-specific call publication facts are adjacent. |
| 4480-5680 | Route1 through Route8 index, record, and validation families | Dominant non-core route analysis inventory. These are the strongest candidates for staged behavior-preserving extraction planning. |
| 5690-5796 | Route index facade and call result identities | Cross-route validation facade; likely high include-risk if moved before route families are classified. |
| 5804-5962 | Terminators, blocks, functions, module | Core model contract; keep central until lower-risk helper and route records are resolved. |
| 5972-6322 | Lookup helpers, inline predicates, render/analysis declarations, print/validate | Public helper tail combines core render hooks with route analysis declarations. |

### `bir.cpp`

| Lines | Cluster | Initial observation |
| --- | --- | --- |
| 10-186 | Anonymous comparison and producer helpers | Private helper algorithms supporting route1 and comparison producer discovery. |
| 189-343 | Route1 source identity and scalar producer index | Route1 scalar producer cluster with local callers and shared `Value` dependency. |
| 353-540 | Route2 select-chain and direct-global dependency | Select-chain helper cluster, with one anonymous recursive helper for direct global dependency. |
| 566-1032 | Route3 memory access records and same-block memory helpers | Cohesive memory-access cluster; caller probe shows route6 reaches into this cluster for call publication source records. |
| 1035-1697 | Route4 publication availability and reference validation | Publication availability cluster; callee probe shows it builds from route1 and route4 current/block-entry records. |
| 1699-2464 | Route5 CFG-edge publication and join-source analysis | Large publication/CFG join cluster adjacent to route4 but distinct in record types. |
| 2478-2588 | `Value` constructors and structured type spelling lookup | Core implementation bodies embedded in the middle of route clusters. |
| 2589-2703 | Render helpers for type, binary opcode, cast opcode | Printer-like string rendering functions currently declared in `bir.hpp` and implemented in `bir.cpp`. |
| 2705-3528 | Call argument source, route6 call-use source indexes, call result source records | Route6 call publication cluster; depends on route1, route2, route3, route4, and call argument helper records. |
| 3564-4053 | Comparison operand and route7 comparison condition indexes | Route7 comparison cluster; caller/callee probe confirms condition index construction feeds materialized condition producer identity. |
| 4181-4421 | Route8 return-chain index and finders | Return-chain cluster, mostly route8-local. |
| 4431-4902 | Route7 and route-index validation facades | Cross-route validation layer bridging route4 and route7 records. |
| 4937-5138 | Fused compare, materialized condition, call result identity, publication routing | Tail helpers used by public query functions; includes call publication routing that depends on call argument relationship, selection availability, and direct-global dependency availability. |

## Dependency Observations

- `bir.hpp` is not only the core BIR model header; it also contains route-index
  analysis records for routes 1-8, local-array proof facts, memory provenance
  authority records, call publication records, route validation facades, and
  print/validate declarations.
- `bir.cpp` has cohesive route clusters, but they are interleaved with true
  core implementations such as `Value` constructors and rendering helpers.
- Route3 memory access is locally cohesive, but `route6_call_argument_publication_source_record`
  directly calls `route3_memory_access_record`, so later movement must account
  for route6-to-route3 coupling.
- Route4 publication availability construction is callable from outside the
  current translation unit or only through declarations not observed as direct
  local callers by the tool. Treat this as an API surface, not dead code.
- Route7 comparison indexing has a small direct callee set, but its records and
  validation facade participate in later fused compare and materialized
  condition queries.
- The route-index facade layer is explicitly cross-route and should be kept
  late in any proposed follow-up sequence.

## Next Analysis Targets

- Classify declarations in `bir.hpp` into core model, route-local analysis,
  memory authority, call publication, validation facade, and printer/render
  surfaces.
- Check adjacent files (`bir_printer.cpp`, `bir_validate.cpp`, and
  `lir_to_bir/`) before proposing destinations, because some current
  declarations may already have a natural owner outside `bir.hpp`.
- Avoid moving `Value`, `Inst`, `Block`, `Function`, and `Module` until lower
  risk route analysis families have been isolated in the cleanup plan.
