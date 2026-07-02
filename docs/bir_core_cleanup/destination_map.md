# BIR Core Cleanup Destination Map

Source idea: `ideas/open/518_bir_core_model_cleanup_umbrella.md`
Plan step: Step 4 - Draft Destination Map

This is an analysis-only destination map. No implementation files were changed.

## Inputs

This map is derived from the existing Step 1-3 artifacts:

- `docs/bir_core_cleanup/structure_snapshot.md`
- `docs/bir_core_cleanup/declaration_inventory.md`
- `docs/bir_core_cleanup/implementation_inventory.md`

No additional clang-tool or raw-text commands were needed for this packet. The
destination choices below reuse the recorded AST-backed symbol counts,
caller/callee probes, type-reference observations, and adjacent owner maps from
those artifacts.

## Destination Principles

- Keep the public BIR model contract central until lower-risk analysis
  families are split around it.
- Prefer existing focused files when they already express the right owner:
  `bir_printer.cpp`, `bir_validate.cpp`, and the existing
  `lir_to_bir/` private lowering split should not be bypassed by new central
  catch-all files.
- Create new BIR route headers or translation units only when moving code into
  an existing file would keep the same monolithic coupling under a new name.
- Treat `lir_to_bir/` as a private lowering subsystem. Do not move public BIR
  model records into that tree unless they are proven lowering-only.
- Move broad core types late, if at all. `Value`, `Inst`, `Block`, `Function`,
  `Module`, `MemoryAddress`, and route-index facade surfaces should remain
  stable while leaf or helper families are extracted.
- Preserve route dependency direction explicitly. A later route extraction must
  expose stable query APIs instead of relying on moved private helpers.

## Existing-File Redistribution Candidates

These families already have plausible destination owners in existing files.
Later cleanup work can target them without inventing new public surfaces, as
long as declarations and API compatibility remain stable.

| Family | Current owner | Proposed destination | Rationale | Constraints |
| --- | --- | --- | --- | --- |
| Printer-only private helpers | `bir_printer.cpp` | Retain in `bir_printer.cpp` | The implementation inventory shows this file already owns quoted text escaping, value rendering, call suffix/source annotations, block labels, memory address formatting, phi observations, function rendering, and `print`. | Do not move route analysis declarations into the printer. Keep printer behavior stable and prove output-sensitive tests if edited later. |
| Public `print(Module, ...)` API | Declaration in `bir.hpp`, body in `bir_printer.cpp` | Keep declaration public; keep body in `bir_printer.cpp` | This is already split correctly: public API in the model header, implementation in the printer TU. | A future smaller print header is possible only after include consumers are counted. |
| Validator-only private helpers | `bir_validate.cpp` | Retain in `bir_validate.cpp` | The validator file already owns private lookup/check helpers for names, values, slots, calls, memory ops, params, returns, and terminators. | Do not collapse route-index validation into module validation until route facade ownership is settled. |
| Public `validate(Module, ...)` API | Declaration in `bir.hpp`, body in `bir_validate.cpp` | Keep declaration public; keep body in `bir_validate.cpp` | Existing ownership is clear and should stay stable. | Behavior-preserving edits require backend validation proof. |
| Public render helper bodies | `bir.cpp` lines 2589-2675 | Candidate for `bir_printer.cpp` or a new narrow render TU | `render_type`, `render_binary_opcode`, and `render_cast_opcode` are printer-like and low to medium risk. | The helpers are public APIs, not printer internals. If moved to `bir_printer.cpp`, ensure non-printer users do not inherit printer-only dependencies. |
| Structured type spelling lookup bodies | `bir.cpp` lines 2493-2579 | Retain with central module/type implementation for now | The declaration inventory marks structured type spelling adjacent to `Global`, `StringConstant`, and `Module` storage. | Do not move into printer or lowering; it is module/type model behavior. |
| LIR-to-BIR pure layout/projection helpers | `lir_to_bir/memory/memory_helpers.hpp` and related memory TUs | Retain in existing memory helper files | `memory_helpers.hpp` is already a narrow private helper surface for projection/layout helpers. | Keep stateful lowering policy on `BirFunctionLowerer`; do not widen the helper header into a policy bucket. |
| Lowering context notes and module orchestration | `lir_to_bir/context.cpp`, `lir_to_bir/module.cpp` | Retain in existing files | Step 3 found `lir_to_bir/` already split by function and state ownership. | Same-module formal pointer publication should remain an implementation owner issue, not a public BIR model split. |

## Possible New Files

These are candidate destinations for later implementation ideas. They are not
created by this analysis packet.

| Candidate destination | Families | Public or private | Why a new file may be better than an existing file | Extraction risk |
| --- | --- | --- | --- | --- |
| `src/backend/bir/bir_route8.cpp` plus optional private route8 header | Route8 return-chain index and finders | Mostly private implementation with public declarations initially remaining in `bir.hpp` | Route8 is the most leaf-like route cluster in Step 3, with route1 identity as its main shared input. Folding it into route1 or route6 would hide a useful boundary. | Low to medium. Stabilize route1 identity input first. |
| `src/backend/bir/bir_route1.cpp` plus optional private route shared header | Route1 scalar producer index and source identity helpers | Private implementation with public query declarations retained until header split is planned | Route1 is foundational for routes 2, 4, 5, 6, 7, and 8. A focused TU can make that dependency explicit without moving broad model types. | Medium. Many later route families consume it, so declaration movement should lag body movement. |
| `src/backend/bir/bir_route2.cpp` | Select-chain and direct-global dependency analysis | Private implementation with public declarations retained initially | Route2 has coherent implementation bodies and a known route6 consumer. A dedicated TU is clearer than pushing these helpers into route6. | Medium. Route6 needs a stable direct-global dependency API. |
| `src/backend/bir/bir_route3_memory.cpp` | Route3 memory-access records and same-block memory source queries | Public route query surface plus private helpers | Route3 is BIR-side memory indexing, distinct from LIR-to-BIR memory lowering. Moving it into `lir_to_bir/memory/` would invert ownership. | Medium to high. Route6 directly calls route3 memory-access record construction today. |
| `src/backend/bir/bir_route4_publication.cpp` | Route4 current-block and block-entry publication availability | Public route query surface plus private helpers | Publication availability is cohesive and not naturally validator-only, printer-only, or lowering-only. | Medium. Route6 and route-index validation rely on route4 records. |
| `src/backend/bir/bir_route5_publication.cpp` | Route5 CFG-edge and join-source publication | Public route query surface plus private helpers | Route5 is a large publication family adjacent to route4 but conceptually separate enough for its own destination. | Medium. It embeds route3 memory access and uses route1/route4 matching concepts. |
| `src/backend/bir/bir_route7_comparison.cpp` | Route7 comparison condition index, comparison operand producers, fused/materialized condition support | Public route query surface plus private helpers | Comparison analysis has a coherent owner, but should not be merged into the cross-route facade. | Medium to high. Materialized condition helpers depend on route-index facade and validation helpers. |
| `src/backend/bir/bir_route6_call_publication.cpp` | Route6 call-use, call-argument publication, call-result source, and publication routing | Public route query surface plus private helpers | Route6 is large enough to deserve an owner, but only after route1-5 inputs are explicit. | High. It composes route1, route2, route3, route4, route5, call ABI, and tail query helpers. |
| `src/backend/bir/bir_route_facade.cpp` | Route-index reference facade, route-specific validation records, facade-backed public query helpers | Public facade implementation | The facade is cross-route glue and should not be hidden inside any one route file. | High and late. Move only after route4 and route7 APIs stabilize. |
| `src/backend/bir/bir_memory_provenance.hpp` and possibly `.cpp` | Memory provenance, object extent/range, dynamic-array, storage authority, static GEP authority | Public model support header | Existing files would either keep monolithic coupling in `bir.hpp` or incorrectly move public memory authority into private lowering. A public memory model header may eventually be the right boundary. | High. `MemoryAddress` crosses instruction payloads, route3 records, and LIR-to-BIR memory lowering. |
| `src/backend/bir/bir_routes.hpp` or narrower per-route public headers | Route record declarations currently packed into `bir.hpp` | Public or semi-public model support headers | New headers may eventually reduce `bir.hpp` size once body extractions prove stable. | Medium to high. Avoid a single giant `bir_routes.hpp` unless it truly reduces include pressure; narrower headers are preferable when dependency direction is known. |

## Retained Central Surfaces

These families should remain in `bir.hpp`/`bir.cpp` during the first cleanup
wave. Some may later move to smaller core headers, but they are not good early
route-cleanup targets.

| Family | Retained destination | Reason |
| --- | --- | --- |
| Forward declarations, `NameTables`, and identity ids | `bir.hpp` | Central identity plumbing for `Module`, `Function`, and `Block`. |
| Type, target, atomic, intrinsic, calling-convention, and ABI vocabulary | `bir.hpp` | Shared public model vocabulary used by instructions, module data, printer, validator, and target emission. |
| `Value` and scalar value identity | `bir.hpp`/central implementation | Heavy fanout and link-visible pointer identity make this late/no-move. |
| Phi, call ABI, params, and local slots | `bir.hpp` | Part of function/block and instruction payload shape. |
| Core instruction payloads and `Inst` variant | `bir.hpp` | This is the public IR contract. Splitting individual payloads before route extraction would create churn. |
| `Block`, `Function`, `Module`, and terminators | `bir.hpp`/central implementation | Core model containers and storage owners. Route and provenance vectors live here today, so route declaration movement must happen first. |
| Global/module records and structured type spelling | `bir.hpp`/central implementation | Public module contract. Only helper bodies might move later after consumers are mapped. |
| `MemoryAddress` in instruction payloads | `bir.hpp` | It is central to load/store, inline asm, intrinsic memory operands, and route3 records. |
| Cross-route validation facade | Retain in `bir.cpp` for now | This bridge should move only after route4 and route7 boundaries are stable. |
| Tail public query helpers | Retain in `bir.cpp` for now | Fused compare, materialized condition, call-result identity, and publication routing compose multiple route families. |

## Family-To-Destination Matrix

| Family | Preferred destination class | First follow-up shape |
| --- | --- | --- |
| Printer private helpers | Existing file | Leave in `bir_printer.cpp`; optionally move public render bodies only after API consumers are checked. |
| Validator private helpers | Existing file | Leave in `bir_validate.cpp`; do not absorb route-index validation yet. |
| Public render helpers | Existing file or new narrow TU | Move bodies out of `bir.cpp` only if public API remains stable and dependencies stay small. |
| Route8 return chain | New focused route TU | Good first route-body extraction candidate after route1 identity dependency is explicit. |
| Route1 scalar producer | New focused route TU | Extract body early enough to support route8/route2/route4 follow-ups, but keep declarations stable. |
| Route2 select-chain | New focused route TU | Extract with route6 direct-global dependency recorded as an API consumer. |
| Route3 memory access | New focused route TU, not `lir_to_bir/memory/` | Extract only with a route3 query API for route6. |
| Route4 publication | New focused route TU | Extract after route1 body boundary exists; keep validation/facade consumers visible. |
| Route5 publication | New focused route TU | Extract after route3 and route4 dependencies are explicit. |
| Route7 comparison | New focused route TU | Extract route7 record construction before moving facade-backed public queries. |
| Route6 call publication | New focused route TU, late | Defer until route1-5 and call ABI inputs are stable. |
| Route-index facade | New facade TU, late | Move after route4/route7 routes settle. |
| Memory provenance and storage authority | Possible new public memory model header, late | Defer until route3 and lowering consumers are mapped with include tests. |
| Local-array proof and semantic GEP records | Possible new public analysis header, late | Defer because `Function` owns storage vectors and memory authority ownership is unsettled. |
| Core model types | Retained central | No early movement. |
| LIR-to-BIR private lowering state | Existing private lowering files | Retain current split; plan separate private-lowering cleanup only if needed. |

## Include And API Risks

- `bir.hpp` is currently the public aggregation point. Moving declarations out
  before implementation bodies are split risks forcing most consumers to include
  both old and new headers with no real dependency reduction.
- Body-only route extraction is likely safer than declaration extraction for
  early follow-ups. Keep public declarations in `bir.hpp` until include
  consumers are counted and the route APIs are stable.
- Route6 currently reaches into route3 and route4 behavior. If route3 or route4
  bodies move first, route6 must call stable public or private route query
  functions, not newly exported implementation details.
- Route5 embeds route3 memory access and uses route1/route4 matching concepts.
  A route5 split should follow route1 and route4 boundary work.
- Route7 public query helpers depend on the route-index facade. Extract route7
  record construction separately from facade-backed materialized-condition
  public queries.
- Moving public render helpers into `bir_printer.cpp` could make non-printer
  users link against the printer TU. A dedicated render TU may be cleaner if
  render helpers are used outside printer output.
- `MemoryAddress`, memory provenance, storage authority, and static GEP
  authority are not lowering-only. Moving them into `lir_to_bir/memory/` would
  make public model consumers depend on a private lowering subsystem.
- `Function` owns vectors of several analysis/provenance records. Declaration
  extraction must account for complete-type requirements, vector storage, and
  inline methods before moving records out of `bir.hpp`.
- A single large `bir_routes.hpp` could become a new monolith. Prefer
  per-route or small family headers only after the dependency order is known.

## Late Or No-Move Regions

- Core model identity, values, instruction payloads, blocks, functions,
  modules, terminators, and global/module records.
- Public memory address and provenance authority families until route3 and
  LIR-to-BIR memory consumers are mapped at include/build level.
- Route6 call publication and call-result source surfaces until route1 through
  route5 inputs become explicit APIs.
- Cross-route validation facade and facade-backed tail query helpers until
  route4 and route7 are stable.
- Stateful `BirFunctionLowerer` methods unless a separate private lowering
  cleanup idea owns that split with compile proof.

## Suggested Extraction Order Signal

This is a planning signal for Step 5, not an implementation instruction:

1. Preserve existing printer and validator ownership; consider only tiny public
   render body movement after consumer checks.
2. Extract route8 body as a low-risk leaf once route1 identity input is stable.
3. Extract route1 body to establish a shared producer-index boundary.
4. Extract route2 and route4 bodies with route6/facade consumers documented.
5. Extract route3 memory and route5 publication only with explicit route3 and
   route4 dependency APIs.
6. Extract route7 record construction separately from facade-backed public
   query helpers.
7. Defer route6, route-index facade, memory provenance headers, local-array
   proof headers, and broad core model headers until earlier splits prove the
   include direction.
