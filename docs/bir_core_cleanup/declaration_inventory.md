# BIR Declaration Family Inventory

Source idea: `ideas/open/518_bir_core_model_cleanup_umbrella.md`
Plan step: Step 2 - Inventory Declaration Families

This is an analysis-only inventory of declarations in `src/backend/bir/bir.hpp`.
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

Header inventory used the flags-based entrypoint because `bir.hpp` is not a
direct compile database translation unit:

```sh
c4c-clang-tool list-symbols src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool type-refs src/backend/bir/bir.hpp Module -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool type-refs src/backend/bir/bir.hpp RouteIndexContext -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool type-refs src/backend/bir/bir.hpp MemoryAccessProvenance -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
c4c-clang-tool type-refs src/backend/bir/bir.hpp Route6CallUseSourceIndex -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
```

The `list-symbols` and `function-signatures` queries succeeded and matched the
Step 1 counts: 132 structs, 80 enums, 200 functions, 15 methods, 69 variables,
7 constructors, 1 type alias, and 3 namespace symbols in `bir.hpp`.
`RouteIndexContext` returned `ok: false` with `type not referenced in
translation unit`; this is not a blocker because the current facade type is
`RouteIndexReferenceFacade`.

Narrow raw-text fallback was used only to anchor declaration line ranges after
the AST pass:

```sh
rg -n "^(struct|enum class|using|inline|std::|bool|const|Route[0-9]|Value|Type|Memory|Module|Function|Block|Inst|void|int)" src/backend/bir/bir.hpp | sed -n '1,260p'
rg -n "(MemoryAddress|ObjectStorage|StorageObject|Provenance|Pointer|Compatibility|Compatible|Select|DirectGlobal|Comparison|Scalar|Producer|Route[0-9]|RouteIndex|render_|print\\(|validate\\()" src/backend/bir/bir.hpp | sed -n '1,260p'
sed -n '420,760p' src/backend/bir/bir.hpp
sed -n '3360,4445p' src/backend/bir/bir.hpp
sed -n '4480,6325p' src/backend/bir/bir.hpp
sed -n '5035,5450p' src/backend/bir/bir.hpp
sed -n '5440,5810p' src/backend/bir/bir.hpp
```

## Declaration Family Table

| Family | Current location in `bir.hpp` | Likely owner | Movement risk | Do-not-move-yet notes |
| --- | --- | --- | --- | --- |
| Forward declarations, name tables, and identity ids | lines 17-84 | Central BIR public model header | High | `Module`, `Function`, and `Block` forward declarations plus `NameTables` are core identity plumbing. Keep central until all route analysis includes are understood. |
| Type, target, atomic, intrinsic, calling-convention, and ABI vocabulary | lines 84-431 | Central BIR public model header | Medium | These enums are shared model vocabulary used by instructions, module data, printer, validator, and target emission. Later extraction would need a stable `types`/ABI vocabulary header, not a route-local owner. |
| `Value` and scalar value identity | lines 439-486 | Central BIR public model header | High | AST type-reference probes in Step 1 showed heavy `Value` fanout. `Value` also carries link-visible pointer identity, so it is a late/no-move candidate. |
| Route1 scalar producer seed records | lines 490-567 and 4480-4569 | Route analysis, likely scalar producer/publication support | Medium | This is route-specific analysis, but it is foundational for route2, route4, route5, route6, route7, and route8 queries. Move only after dependency direction is made explicit. |
| Phi, call ABI, params, and local slots | lines 571-637 | Function/block core model | High | These are part of callable function shape and instruction payloads. They should remain with `Function`, `Block`, `CallInst`, and `PhiInst` until a true core-model header split exists. |
| Local array proof, range, semantic GEP, and scalar-local-load records | lines 637-2373, plus `Function` storage at 5917-5952 | Local-array provenance analysis, probably LIR-to-BIR/local-array publication support | Medium to high | The records are analysis-heavy and large, but `Function` owns vectors of these records. Do not move before Step 3 maps implementations and before an include strategy avoids dragging core `Function` through route-local headers. |
| Global/module records and structured type spelling | lines 3394-3468 and `Module` storage at 5962-5969 | Central module/global model, with possible structured-type spelling helper owner | Medium | `Global`, `StringConstant`, structured type spelling, and `Module` are public model contract. Only spelling helpers may become lower-risk later; global/module records should stay central. |
| Memory-address, memory-provenance, object-extent, byte-range, dynamic-array, and storage authority records | lines 3470-3654, with uses in `MemoryAddress` and route3 at 4757 | Memory model/public address surface, possibly a future memory provenance header | High | This family is not route-local: `MemoryAddress` appears in instruction payloads, inline asm, intrinsic memory operands, and route3 records. Object-storage/extent authority is central to pointer-value safety, so split only after consumers are mapped. |
| Global static GEP authority and semantic GEP records | lines 3657-3931, plus `Function` storage at 5945-5948 | Global memory provenance analysis | Medium | Strong candidate for a later focused analysis header, but it shares memory verdict enums and `Function` storage. Do not move until memory provenance ownership is settled. |
| Core instruction definitions and `Inst` variant | lines 3933-4343 | Central instruction model header | High | `BinaryInst`, `SelectInst`, `CastInst`, `PhiInst`, `CallInst`, local/global load/store, inline asm, intrinsic operation, and `Inst` define the public IR contract. Keep central until route analyses are extracted around them. |
| Select-chain and direct-global dependency records | lines 4573-4676, plus call-argument dependency mirror at 4196-4209 | Route2 select-chain analysis with route6 call-publication dependency | Medium | Route2 has a natural owner, but route6 consumes direct-global select-chain facts. Record the route2-to-route6 contract before moving declarations. |
| Route3 memory-access analysis records | lines 4679-4882 | Route3 memory-access analysis | Medium | Step 1 caller probes showed route6 call publication calls into route3 memory access. Route3 extraction must preserve route6 access to memory-source records. |
| Route4 publication availability records | lines 4885-5033 and validation declarations at 6249-6258 | Route4 publication analysis plus validation facade | Medium | Route4 has cohesive declarations, but route index validation and route6 publication sources depend on its records. Move only with facade and route6 dependency notes. |
| Route5 CFG-edge and join-source publication records | lines 5043-5244 | Route5 publication analysis | Medium | Mostly route-local, but route5 records embed route3 memory access and feed route6 publication-source records. Do not move without tracking those cross-route edges. |
| Route6 call-use, call-argument source, call-result source, and publication-source records | lines 4113-4261, 5246-5439, and declarations at 6071-6164 | Route6 call publication analysis | High | This family bridges call instruction payload, route1 producers, route2 direct-global dependency, route3 memory source, route4 current-block publication, and route5 edge publication. It is a late move until its dependency fan-in is isolated. |
| Comparison/scalar producers and route7 comparison condition records | lines 5442-5614 and declarations at 6167-6219 | Route7 comparison analysis | Medium | Cohesive route7 owner, but shared `ComparisonOperandProducer` and fused/materialized helper declarations interact with route1 scalar producer concepts and validation facade. Move after route1 and facade boundaries are clear. |
| Route8 return-chain records | lines 5617-5688 and declarations at 6221-6247 | Route8 return-chain analysis | Low to medium | This appears more self-contained than route6/route7, with dependency on `Route1SourceValueIdentity`. It may be an earlier extraction candidate once implementation ownership is verified. |
| Route-index reference facade and route-specific validation records | lines 5690-5788 and declarations at 6249-6310 | Cross-route validation facade, possibly adjacent to `bir_validate.cpp` after mapping | High | Facade joins route4 and route7 indexes and should remain late. Moving it before route4/route7 ownership stabilizes would create include churn and stale-reference risk. |
| Call-result identity helpers | lines 5790-5802 and declarations at 6312-6319 | Route6/call-result query surface | Medium | These helpers are call-specific but exposed near the public tail. Treat as route6-adjacent until Step 3 maps callers and implementation bodies. |
| Terminators, `Block`, `Function`, `Module`, and formal pointer authority | lines 5804-5969 | Central BIR public model header | High | This is the core model contract and owns vectors of several route-local/provenance records. Do not move until lower-risk route declarations are disentangled from `Function`. |
| Compatibility helpers and inline model predicates | lines 5972-6066 | Mixed: local-array lookup, route-independent predicates, and compatibility parsing | Medium | `is_compare_opcode`, `binary_operand_type`, and `select_compare_type` may be low-risk helpers, but local-array lookup and immediate-return parsing touch `Function`. Keep until Step 3 maps implementation/caller clusters. |
| Printer and render surfaces | lines 6068-6070 and `print` at 6321 | Existing `bir_printer.cpp` surface | Low to medium | Declarations already point at printer/render behavior, but `render_type`, opcode renderers, and `print(Module)` are public APIs. They are plausible existing-file owners after include-cycle review. |
| Public validation facade | line 6322 plus route validation declarations at 6249-6310 | Existing `bir_validate.cpp` for module validation; route-index validation remains cross-route | Medium to high | `validate(Module, error)` naturally belongs to validation, but route-index validation declarations are part of route analysis integrity. Do not collapse them into module validation without preserving route-specific ownership. |

## Ownership Notes

- The only `Inst` alias is the central variant at line 4346. Moving individual
  instruction payloads away from that alias would be high churn unless a new
  stable instruction-model header is introduced.
- There is no distinct object-storage declaration family by that exact name in
  `bir.hpp`; current storage authority is expressed through `MemoryObjectExtent`,
  `MemoryByteRange`, `MemoryAccessProvenance`, `MemoryAddress`,
  `LocalSlotStorageKind`, and `Function`/`Module` storage vectors.
- `MemoryAccessProvenance` type references were found in the range-proof helper
  functions, `MemoryAddress`, and `Route3MemoryAccessRecord`, confirming that
  memory authority crosses core instruction payloads and route3 analysis.
- `Route6CallUseSourceIndex` references cluster in the route6 public query
  declarations, confirming that route6 has a clear surface but broad fan-in.
- The public tail mixes render, call publication, comparison, return-chain,
  route-index validation, call-result identity, print, and validate declarations;
  Step 3 should map implementation bodies before proposing any tail split.

## Candidate Movement Sequence Signal

Lower-risk later candidates appear to be printer/render declarations, route8
return-chain declarations, and possibly route5 after route3 coupling is mapped.
Late/no-move candidates are `Value`, `Inst`, `Block`, `Function`, `Module`,
memory provenance/address authority, route6 call publication, and the
route-index validation facade.
