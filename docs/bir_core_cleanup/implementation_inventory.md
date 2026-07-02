# BIR Implementation Family Inventory

Source idea: `ideas/open/518_bir_core_model_cleanup_umbrella.md`
Plan step: Step 3 - Inventory Implementation Families

This is an analysis-only inventory of implementation bodies in
`src/backend/bir/bir.cpp`, `src/backend/bir/bir_printer.cpp`,
`src/backend/bir/bir_validate.cpp`, and `src/backend/bir/lir_to_bir/`.
No implementation files were changed.

## Tooling And Fallback

The clang tools were available on `PATH`:

```sh
command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb
```

Result:

```text
/home/vscode/.local/bin/c4c-clang-tool
/home/vscode/.local/bin/c4c-clang-tool-ccdb
```

Implementation inventory used the compile database entrypoint:

```sh
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir_printer.cpp build/compile_commands.json
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir_validate.cpp build/compile_commands.json
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/lir_to_bir/module.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir/memory/coordinator.cpp build/compile_commands.json
for f in src/backend/bir/bir.cpp src/backend/bir/bir_printer.cpp src/backend/bir/bir_validate.cpp src/backend/bir/lir_to_bir/*.cpp src/backend/bir/lir_to_bir/memory/*.cpp; do c4c-clang-tool-ccdb list-symbols /workspaces/c4c/$f build/compile_commands.json; done
for sym in route3_memory_access_record route4_build_publication_availability_index route5_build_edge_join_source_index route6_call_argument_publication_source_record find_materialized_condition_producer_identity route8_build_return_chain_index; do c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/bir.cpp $sym build/compile_commands.json; c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/bir/bir.cpp $sym build/compile_commands.json; done
for sym in route6_build_call_use_source_index find_call_result_source_identity find_call_argument_publication_source_routing route_index_reference_facade route_index_validate_materialized_condition_reference; do c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/bir.cpp $sym build/compile_commands.json; c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/bir/bir.cpp $sym build/compile_commands.json; done
```

The `memory/coordinator.cpp` `function-signatures` query returned an empty
result even though `list-symbols` found one `BirFunctionLowerer` method. This
was treated as a clang-tool limitation for that file only; `list-symbols` was
used for the memory implementation map.

Narrow raw-text fallback was used only after AST-backed queries to anchor line
ranges, file sizes, and private lowering surfaces:

```sh
wc -l src/backend/bir/bir.cpp src/backend/bir/bir_printer.cpp src/backend/bir/bir_validate.cpp src/backend/bir/lir_to_bir/*.cpp src/backend/bir/lir_to_bir/memory/*.cpp
rg -n "^(std::optional|bool|void|bir::|Lowering|Analyze|Module|Type|Function|class|struct|enum|using|const)" src/backend/bir/lir_to_bir/lowering.hpp | sed -n '1,220p'
sed -n '1,260p' src/backend/bir/lir_to_bir/lowering.hpp
sed -n '260,620p' src/backend/bir/lir_to_bir/lowering.hpp
sed -n '620,1040p' src/backend/bir/lir_to_bir/lowering.hpp
sed -n '1,180p' src/backend/bir/lir_to_bir/memory/memory_helpers.hpp
sed -n '1,220p' src/backend/bir/lir_to_bir/memory/memory_types.hpp
```

## Implementation Shape Summary

| File or region | Size or AST count | Implementation family | Ownership observation |
| --- | --- | --- | --- |
| `bir.cpp` | 5158 lines; 157 functions, 11 methods, 1 struct, 9 type aliases | Core BIR query and route-index implementation | Contains cohesive route clusters, but also central `Value` constructors, structured type lookup, and render helpers. |
| `bir_printer.cpp` | 455 lines; 12 functions | Printer-only rendering | Already owns module/function printing and local render helpers for call annotations, memory addresses, blocks, and phi observations. |
| `bir_validate.cpp` | 920 lines; 34 functions, 2 structs | Validator-only module integrity checks | Already owns `validate(Module, error)` and private lookup/check helpers for names, values, slots, calls, memory ops, params, returns, and terminators. |
| `lir_to_bir/*.cpp` | 9419 lines across non-memory lowering files | LIR-to-BIR lowering surfaces | Already split by analysis, types, globals, initializers, aggregate ABI, call ABI, scalar, CFG, calling, context, and module orchestration. |
| `lir_to_bir/memory/*.cpp` | 10315 lines across memory files | LIR-to-BIR memory/provenance lowering | Already split into addressing, coordinator, intrinsics, local GEP, local slots, provenance, and value materialization. |
| `lir_to_bir/lowering.hpp` | private declaration surface | Stateful lowerer API and split-TU declarations | `BirFunctionLowerer` remains the state owner; many declarations are implementation-only and not public BIR model contract. |
| `lir_to_bir/memory/memory_helpers.hpp` | private helper declaration surface | Pure layout/projection helper declarations | Existing narrow owner for shared memory projection helpers. Should not absorb stateful policy. |
| `lir_to_bir/memory/memory_types.hpp` | private memory type surface | Lowering side tables and local/global pointer records | Stores route-local lowering state types; not a public BIR model owner. |

## Implementation Family Map

| Family | Current implementation | Caller/callee observations | Movement risk |
| --- | --- | --- | --- |
| Anonymous pure comparison/producer helpers | `bir.cpp` lines 10-186 | Local helpers feed comparison producer discovery and route1 producer facts. | Low to medium: extractable only with route1/route7 boundary clarity. |
| Route1 scalar producer index | `bir.cpp` lines 189-343 plus integer constant evaluation at 2242-2488 | Route4 and route6 builders call `route1_build_producer_index`; route5/route7/route8 record finders depend on route1 source identity. | Medium: foundational route dependency, not a standalone leaf. |
| Route2 select-chain and direct-global dependency | `bir.cpp` lines 353-540 | Route6 publication routing uses direct-global select-chain dependency helpers. | Medium: coherent family, but route6 consumes it. |
| Route3 memory-access records and same-block memory source queries | `bir.cpp` lines 566-1032 | `route3_memory_access_record` callees are route3-local plus `route1_source_value_identity`; callers include route3 builders/finders and `route6_call_argument_publication_source_record`. | Medium to high: route6 dependency must be an explicit API if moved. |
| Route4 publication availability and reference validation | `bir.cpp` lines 1035-1697 | Builder calls route1 and route4 record constructors. Public lookup/validation helpers are API-like even when direct local callers are not reported by clang-tool. | Medium: cohesive but shared by route6 and facade validation. |
| Route5 CFG-edge and join-source publication | `bir.cpp` lines 1699-2238 | Builder calls route5 records and block lookup helpers; finders call route1 identity and route4 block matching/value matching helpers. | Medium: more local than route6, but not independent of route1/route4 semantics. |
| Core `Value` and structured type methods | `bir.cpp` lines 2493-2579 | `Value` constructors are central model bodies; structured spelling lookup belongs with module/type model. | Late/no-move: public model contract and broad fanout. |
| Public render helpers | `bir.cpp` lines 2589-2675 | `render_type`, `render_binary_opcode`, and `render_cast_opcode` are printer-like but public helpers declared in `bir.hpp`. | Low to medium: plausible printer/render owner, but public API and include direction must be preserved. |
| Route6 call-use, call-argument publication, and call-result source | `bir.cpp` lines 2705-3528 | `route6_call_argument_publication_source_record` calls route6 source records, publication routing, route3 memory records, route4 current-block publication, and route6 source-kind helpers. `route6_build_call_use_source_index` calls all route6 record constructors. | High: largest cross-route fan-in and publication policy surface. |
| Route7 comparison condition and materialized condition | `bir.cpp` lines 3564-4053 and 4431-5019 | `find_materialized_condition_producer_identity` calls route7 index building, route-index facade, materialized-condition validation, and operand-public conversion. | Medium to high: cohesive comparison owner, but facade and public query helpers are coupled. |
| Route8 return-chain index and finders | `bir.cpp` lines 4071-4421 | Builder calls only return-chain publication helpers in the selected probe; records depend on route1 identity and value-key matching. | Low to medium: best early route extraction candidate after route1 dependency is stabilized. |
| Cross-route validation facade | `bir.cpp` lines 4786-4902 | Facade callers include fused compare and materialized condition helpers. Validation helpers bridge route4 and route7. | Late/no-move: should move only after route4/route7 APIs are stable. |
| Tail public query helpers | `bir.cpp` lines 4937-5138 | Fused compare, materialized condition, call-result identity, and publication routing compose route6/route7/facade helpers. | Medium to high: query surface, not private route implementation only. |
| Printer-only helpers | `bir_printer.cpp` lines 11-444 | `escape_quoted_text`, `render_value`, call suffix/source annotations, block labels, memory addresses, phi observations, function rendering, and `print`. | Low: already isolated; later cleanup can move render helpers here if public API is handled. |
| Validator-only helpers | `bir_validate.cpp` lines 13-723 | Private `fail`, lookup helpers, named value/type checks, instruction checks, local slot and link-name checks, and `validate`. | Low for validator internals; medium for route-index validation because those bodies currently live in `bir.cpp`, not this file. |
| LIR-to-BIR type/layout/global initializer helpers | `lir_to_bir/types.cpp`, `globals.cpp`, `global_initializers.cpp`, `call_abi.cpp`, `aggregate.cpp` | Private lowering helpers are declared through `lowering.hpp` and mostly owned by functional split files. | Low to medium: already split; cleanup should avoid re-centralizing into `bir.hpp`. |
| LIR-to-BIR scalar/CFG/calling/module orchestration | `lir_to_bir/scalar.cpp`, `cfg.cpp`, `calling.cpp`, `module.cpp` | `module.cpp` owns lowering orchestration and same-module formal pointer publication; `calling.cpp` owns inline asm and call lowering surfaces. | Medium to high: stateful `BirFunctionLowerer` methods and target-specific policies share private state. |
| LIR-to-BIR memory/publication/provenance implementation | `lir_to_bir/memory/*.cpp` | Memory files own pointer provenance, local/global GEPs, dynamic arrays, local slots, intrinsics, and value materialization. | Medium to high: already split but heavily stateful; do not move public BIR memory authority records here without API review. |

## Route-Specific Memory, Publication, And Comparison Notes

Route3 is the BIR-side memory-access index, not the LIR-to-BIR memory lowerer.
Its implementation constructs access records and same-block source records from
already-lowered BIR blocks. Route6 consumes `route3_memory_access_record` while
building call-argument publication sources, so a later route3 extraction needs
a stable route3 query API rather than direct private helper reuse.

Route4 and route5 are publication availability families. Route4 is closer to a
general current-block/block-entry publication index and route5 is CFG-edge and
join-source publication. Route5 is more self-contained, but selected probes show
it still uses route1 source identity and route4 block/value matching concepts.

Route6 is the highest-risk publication implementation. It joins route1 producer
facts, route2 direct-global dependency, route3 memory source records, route4
current-block publication, route5 edge/join publication, call ABI lane records,
and call-result identity helpers. It should remain late until the lower-risk
route APIs become explicit.

Route7 is the comparison implementation family. The condition index itself is
cohesive, but public helpers such as fused compare producer facts and
materialized condition producer identity depend on the route-index facade and
validation helpers.

Route8 is the most plausible route-index leaf. It primarily publishes return
chain records and finders, with route1 identity as the important shared input.

## LIR-To-BIR-Only Surfaces

The `lir_to_bir/` implementation family is already a private lowering layer,
not part of the public BIR core model. The public entrypoints are
`make_lowering_context`, `analyze_module`, and `lower_module`; most other
declarations in `lowering.hpp` are split-TU implementation contracts.

The non-memory lowering files have these implementation owners:

| File | Owner |
| --- | --- |
| `analysis.cpp` | module analysis and instruction counting |
| `types.cpp` | type lowering, structured layout, aggregate layout, typed operand parsing |
| `globals.cpp` | global type/address lowering and known global resolution |
| `global_initializers.cpp` | scalar, byte string, array, aggregate, and pointer initializer lowering |
| `call_abi.cpp` | call argument/return ABI and structured signature lowering |
| `aggregate.cpp` | byval aggregate param and local aggregate slot copy/materialization |
| `scalar.cpp` | scalar values, casts, binary ops, compares, and select-chain lowering |
| `cfg.cpp` | block lookup, branch-chain following, phi plans |
| `calling.cpp` | call lowering, inline asm, varargs/HFA handling, runtime intrinsics |
| `module.cpp` | module orchestration, target stack pressure, same-module formal pointer provenance publication, lowering failure notes |
| `context.cpp` | lowering context note construction |

The memory lowering files have these implementation owners:

| File | Owner |
| --- | --- |
| `memory/addressing.cpp` | local/global GEP address resolution, aggregate projection, dynamic global arrays |
| `memory/coordinator.cpp` | dispatch from memory-family LIR instructions to scalar/local memory lowering |
| `memory/intrinsics.cpp` | memcpy/memset intrinsic lowering, local/global leaf copy helpers |
| `memory/local_gep.cpp` | local aggregate and pointer-array GEP analysis |
| `memory/local_slots.cpp` | local slot alloc/load/store, scalar array slots, dynamic local aggregate load/store |
| `memory/provenance.cpp` | pointer-value provenance, global pointer slots, addressed pointer loads/stores |
| `memory/value_materialization.cpp` | initializer values and dynamic global/pointer array value materialization |

## Dependency Observations

- `bir.cpp` route implementation bodies are grouped well enough for staged
  extraction planning, but route order matters more than line adjacency.
- The strongest direct couplings observed by AST probes are route6 to route3,
  route6 to route4, route7 public queries to the route-index facade, and
  route5 to route1/route4 matching helpers.
- `bir_printer.cpp` and `bir_validate.cpp` are already clean implementation
  owners for printer-only and validator-only internals. The remaining question
  is whether public declarations and small public render helpers should point
  more clearly at those owners in a later behavior-preserving idea.
- `lir_to_bir/` should be treated as an existing private lowering subsystem.
  Moving public BIR model records into that tree would invert ownership unless
  the records are proven to be lowering-only.
- `memory_helpers.hpp` is a good precedent for a narrow private helper surface:
  it declares pure shared layout/projection helpers without taking stateful
  lowerer policy away from `BirFunctionLowerer`.

## Movement Risk Classification

Low-risk later cleanup candidates:

- Printer-only private helpers already in `bir_printer.cpp`.
- Validator-only private helpers already in `bir_validate.cpp`.
- Public render helper bodies in `bir.cpp`, if the public API remains stable.
- Route8 return-chain implementation after route1 identity dependency is
  represented as an explicit input.

Medium-risk later cleanup candidates:

- Route1 scalar producer implementation, because many later routes depend on
  it.
- Route2 select-chain implementation, because route6 uses direct-global
  dependency facts.
- Route4 publication availability and route5 CFG-edge/join publication, after
  route6 and route-index facade dependencies are documented.
- Route7 comparison condition indexing, after facade and public query helpers
  are kept separate from private record construction.
- Pure LIR-to-BIR layout/projection helper declarations, if any split keeps
  `lowering.hpp` as the state owner and avoids widening `memory_helpers.hpp`.

Late or no-move regions:

- `Value`, `Inst`, `Block`, `Function`, `Module`, and structured type lookup
  bodies that define the public BIR model contract.
- Memory address/provenance authority records that cross core instruction
  payloads, route3 records, and LIR-to-BIR memory lowering.
- Route6 call publication and call-result source implementation because it
  composes route1, route2, route3, route4, route5, call ABI, and tail query
  helpers.
- Cross-route validation facade and materialized-condition/call publication
  tail helpers.
- Stateful `BirFunctionLowerer` memory/provenance methods unless a later idea
  is specifically scoped to a private lowering split with build proof.
