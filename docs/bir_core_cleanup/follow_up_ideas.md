# BIR Core Cleanup Follow-Up Ideas

Source idea: `ideas/open/518_bir_core_model_cleanup_umbrella.md`
Plan step: Step 5 - Produce Staged Follow-Up Ideas

This is an analysis-only staging artifact. No implementation files were
changed, and no new source idea files were created.

## Inputs

This staged list is derived from the existing Step 1-4 artifacts:

- `docs/bir_core_cleanup/structure_snapshot.md`
- `docs/bir_core_cleanup/declaration_inventory.md`
- `docs/bir_core_cleanup/implementation_inventory.md`
- `docs/bir_core_cleanup/destination_map.md`

No additional clang-tool or raw-text commands were needed for this packet.

## Staging Rules

- Keep idea 422 producer capability work separate from file-organization
  cleanup. These follow-ups may preserve, expose, or move existing BIR analysis
  behavior, but they must not add new producer semantics.
- Keep body-only route extraction separate from later declaration/header
  extraction. Early route work should move implementation bodies while leaving
  the public `bir.hpp` API stable unless a specific later idea owns header
  changes.
- Prefer small slices that can be rejected by review if they broaden scope,
  change behavior, or create a new monolithic route bucket.
- Retain broad core model types and public memory authority surfaces until
  lower-risk route implementation movement proves dependency direction.

## Recommended Execution Order

### 1. Preserve Printer And Validator Owners, Then Isolate Public Render Bodies

Owned files:

- `src/backend/bir/bir.cpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/bir/bir.hpp` only if declarations need comments or include
  neutrality; avoid declaration movement in the first slice
- focused backend printer/validator tests if they already cover rendered output

Non-goals:

- Do not move route analysis declarations or records into `bir_printer.cpp`.
- Do not change `print(Module, ...)` or `validate(Module, ...)` behavior.
- Do not fold route-index validation helpers into module validation.
- Do not start route body extraction in this slice.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused proof should include backend BIR printer and validator tests, or the
  nearest repo-native `^backend_` subset if no narrower target exists.
- If public render helper linkage changes, run at least one object-emission or
  backend subset that links non-printer users.

Reviewer reject signals:

- Render text changes without an explicit approved expectation update.
- New dependencies from non-printer code on printer-only implementation details.
- Moving route declarations, route validation, or public model records under a
  printer or validator owner.
- Combining render cleanup with route extraction.

Why behavior-preserving:

- The candidate work only relocates existing render helper bodies or keeps
  existing printer/validator ownership explicit. Public names, signatures,
  printed spelling, validation diagnostics, and route analysis behavior should
  remain unchanged.

### 2. Extract Route8 Return-Chain Bodies Only

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route8.cpp` if selected by the implementation idea
- build metadata required to compile the new translation unit
- focused backend route/return-chain tests

Non-goals:

- Do not move route8 declarations out of `bir.hpp`.
- Do not alter `Route1SourceValueIdentity` or scalar producer behavior.
- Do not touch route6 call publication, route7 comparison, or facade helpers.
- Do not implement or modify idea 422 producer capability.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof should cover return-chain analysis and any callers of
  route8 public queries.
- Include a link-time proof through the backend subset because this slice adds
  or repoints route8 definitions.

Reviewer reject signals:

- Header splitting in the same slice as body extraction.
- Route8 changes that alter returned identities, ordering, optional/nullopt
  decisions, or diagnostics.
- New direct dependencies from route8 bodies into route6 or facade internals.
- Expectation rewrites used as proof of cleanup success.

Why behavior-preserving:

- Route8 was identified as the most leaf-like route family. Moving the same
  function bodies into a focused translation unit while preserving public
  declarations and call sites should only change physical ownership, not the
  return-chain records produced.

### 3. Extract Route1 Scalar Producer Bodies As The Shared Route Boundary

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route1.cpp` and, only if necessary, a private route
  helper header
- build metadata required to compile the new translation unit
- focused backend producer/publication/comparison tests

Non-goals:

- Do not change scalar producer semantics, integer constant evaluation, or
  source identity matching.
- Do not move `Value` declarations or constructors.
- Do not move route declarations out of `bir.hpp`.
- Do not add new producer capability for idea 422.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must cover existing producer-index consumers, including
  route4, route5, route6, route7, and route8 paths when available.
- Run `git diff --check` because this slice is mostly physical movement and
  should stay mechanically clean.

Reviewer reject signals:

- Any semantic change to producer identity selection, constant folding, or
  value matching.
- Public API churn beyond what is required for body relocation.
- Combining route1 extraction with route2, route4, route6, or route7 behavior
  changes.
- New named-case checks that make one known testcase pass.

Why behavior-preserving:

- Route1 is a shared dependency for later route extraction. The intended slice
  moves existing bodies behind the same declarations so downstream route
  consumers continue to call the same API and receive the same producer facts.

### 4. Extract Route2 Select-Chain Bodies With Route6 Consumer Contract Recorded

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route2.cpp`
- optional private route helper header only for existing route2 helper
  declarations
- build metadata required to compile the new translation unit
- focused backend select-chain and call-publication tests

Non-goals:

- Do not move route2 public declarations out of `bir.hpp`.
- Do not change direct-global dependency classification.
- Do not edit route6 publication policy, call ABI behavior, or idea 422
  producer behavior.
- Do not fold route2 helpers into route6.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must include route6 call-publication coverage that
  consumes route2 direct-global dependency facts.
- If no narrow route2 test target exists, run the delegated `^backend_` subset.

Reviewer reject signals:

- Route6 starts reaching into newly private route2 implementation details
  instead of calling a stable existing query/helper.
- Direct-global select-chain availability changes.
- Header extraction appears in the body-only slice.
- Tests or expectations are weakened.

Why behavior-preserving:

- The destination map identifies route2 as cohesive but consumed by route6.
  Keeping declarations stable and documenting route6 as a consumer lets the same
  dependency remain visible while only the implementation owner changes.

### 5. Extract Route4 Publication Bodies Before Route5 And Route6

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route4_publication.cpp`
- optional private route helper header only if needed for existing helpers
- build metadata required to compile the new translation unit
- focused backend publication and route-index validation tests

Non-goals:

- Do not move route4 public declarations out of `bir.hpp`.
- Do not move route-index facade or route-specific validation records.
- Do not alter route6 publication source selection.
- Do not combine route4 extraction with route5 publication extraction.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must cover current-block and block-entry publication
  availability plus any route-index validation consumers.
- Include route6 publication coverage when the moved route4 bodies are consumed
  by route6 helpers.

Reviewer reject signals:

- Route-index facade or validation behavior changes in a body extraction slice.
- Publication availability records change shape, order, or optionality.
- Route6 gains new private implementation coupling to route4.
- Expectation rewrites substitute for proving unchanged behavior.

Why behavior-preserving:

- Route4 is a cohesive publication availability family. Moving its bodies after
  route1 establishes a route boundary while preserving public declarations and
  the same publication queries for facade and route6 consumers.

### 6. Extract Route3 Memory-Access Bodies With Explicit Route6 Access

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route3_memory.cpp`
- optional private route helper header only if needed to preserve existing
  helper visibility
- build metadata required to compile the new translation unit
- focused backend memory-access and call-publication tests

Non-goals:

- Do not move route3 into `src/backend/bir/lir_to_bir/memory/`.
- Do not change `MemoryAddress`, memory provenance, object extent, byte-range,
  or storage authority declarations.
- Do not change route6 call-argument publication source policy.
- Do not split public memory model headers in this slice.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must include same-block memory access/source tests and
  route6 call-publication tests that consume route3 records.
- If memory provenance tests exist separately, include them only as consumers,
  not as justification for changing memory authority semantics.

Reviewer reject signals:

- Public memory authority records are moved into private lowering files.
- Route6 loses access to the same route3 memory-source facts or starts using
  duplicated logic.
- Memory access record construction semantics change.
- The slice attempts public header extraction together with body movement.

Why behavior-preserving:

- Route3 is BIR-side indexing over already-lowered blocks. The intended work
  relocates existing record construction and query bodies while preserving the
  same public route3 surface and the same route6 access to route3 facts.

### 7. Extract Route5 Publication Bodies After Route3 And Route4 Boundaries

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route5_publication.cpp`
- optional private route helper header only for existing helper declarations
- build metadata required to compile the new translation unit
- focused backend CFG-edge, join-source, and publication tests

Non-goals:

- Do not move route5 declarations out of `bir.hpp`.
- Do not change route1 identity, route3 memory records, or route4 matching
  behavior.
- Do not edit route6 publication policy.
- Do not combine route5 extraction with route6 call publication extraction.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must cover edge and join-source publication behavior.
- Include route6 call-publication coverage if route6 consumes route5 records in
  the same linked subset.

Reviewer reject signals:

- Duplicated route3 or route4 matching logic appears in route5.
- Join-source or CFG-edge record ordering/selection changes.
- Route5 extraction requires declaration movement or public API changes.
- Tests are rewritten to accept weaker publication contracts.

Why behavior-preserving:

- Route5 has a distinct publication owner but depends on route1, route3, and
  route4 concepts. Extracting it only after those boundaries are stable should
  preserve behavior while reducing `bir.cpp` concentration.

### 8. Extract Route7 Record Construction, Not Facade-Backed Public Queries

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route7_comparison.cpp`
- optional private route helper header for existing route7-local helpers
- build metadata required to compile the new translation unit
- focused backend comparison and materialized-condition tests

Non-goals:

- Do not move the route-index facade in this slice.
- Do not move facade-backed materialized-condition or fused-compare public
  query helpers unless a separate idea owns that later boundary.
- Do not change comparison operand producer semantics.
- Do not add idea 422 producer behavior.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must include comparison condition indexing and
  materialized-condition consumers.
- Keep a link-time backend subset because public query helpers remain in
  another translation unit.

Reviewer reject signals:

- Facade and route7 record construction are moved as one broad slice.
- Materialized-condition producer identity changes.
- Comparison operand public/private conversion changes without a semantic
  source idea.
- Header splitting is mixed into body-only movement.

Why behavior-preserving:

- Route7 record construction is cohesive, but facade-backed public queries are
  cross-route. Splitting only record construction keeps public behavior stable
  while avoiding premature facade ownership changes.

### 9. Extract Route6 Call Publication Only After Route1-5 Are Stable

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route6_call_publication.cpp`
- optional private route helper header for existing route6-local helpers
- build metadata required to compile the new translation unit
- focused backend call-publication, call-result, and publication-routing tests

Non-goals:

- Do not change call ABI lowering or LIR-to-BIR call generation.
- Do not implement idea 422 producer capability.
- Do not move route6 declarations out of `bir.hpp`.
- Do not move route-index facade or memory provenance headers in this slice.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must cover call-use source indexes, call-argument
  publication sources, call-result source identity, and publication routing.
- Because route6 composes many routes, the supervisor should consider the full
  `^backend_` subset even if a narrower call-publication subset passes.

Reviewer reject signals:

- Any changed call-publication source selection, call-result identity, or route
  dependency behavior.
- Route6 duplicates route1-route5 logic instead of using stable APIs.
- The slice expands into call ABI lowering or idea 422 producer work.
- Header extraction or facade movement appears in the same patch.

Why behavior-preserving:

- Route6 is high-risk because it composes route1 through route5 and call ABI
  facts. Waiting until those inputs are stable means the slice can be a pure
  owner move of existing route6 bodies and not a semantic rewrite.

### 10. Move Cross-Route Facade Bodies As A Late Glue Slice

Owned files:

- `src/backend/bir/bir.cpp`
- new `src/backend/bir/bir_route_facade.cpp`
- optional private route facade header only if needed for existing helper
  declarations
- build metadata required to compile the new translation unit
- focused backend route-index validation, fused-compare, and
  materialized-condition tests

Non-goals:

- Do not move core route declarations out of `bir.hpp`.
- Do not alter route4, route7, fused compare, or materialized-condition
  semantics.
- Do not collapse facade validation into `bir_validate.cpp`.
- Do not combine this with route6 extraction.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must include route-index reference facade validation,
  fused compare, and materialized-condition consumers.
- The supervisor should consider `^backend_` if multiple prior route body
  moves have landed since the last broader proof.

Reviewer reject signals:

- Facade movement changes validation failures, source identity, or
  materialized-condition decisions.
- The facade becomes a new catch-all for unrelated route bodies.
- Route4 or route7 APIs are changed to satisfy the move.
- Public declarations are split without a separate header idea.

Why behavior-preserving:

- The facade is cross-route glue. Once route4 and route7 body owners are
  stable, moving only facade bodies can preserve the same calls and validation
  outcomes while removing the remaining glue from `bir.cpp`.

### 11. Split Route Declarations Into Narrow Headers After Body Moves

Owned files:

- `src/backend/bir/bir.hpp`
- new narrow public or semi-public route headers, if selected
- route implementation files created by earlier body-only ideas
- include sites required by compile errors
- build metadata only if new sources or install surfaces require it

Non-goals:

- Do not move implementation bodies in this header-focused slice.
- Do not create one large replacement monolith such as a catch-all
  `bir_routes.hpp` unless review proves it reduces include pressure.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`.
- Do not change route semantics or public query names.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Include-level proof should compile all backend targets that include
  `bir.hpp`.
- Focused backend route tests should run after include churn to catch ODR,
  linkage, and missing-declaration issues.

Reviewer reject signals:

- Header split forces broad consumers to include many new headers without
  reducing coupling.
- Complete-type requirements for `Function` vectors are broken or hidden behind
  fragile forward declarations.
- Public API names, namespaces, or signatures change.
- Header work is mixed with semantic or body movement.

Why behavior-preserving:

- After body owners are stable, declarations can be reorganized while
  preserving names, signatures, namespaces, and storage layout. The slice should
  only change where declarations are included from, not what they mean.

### 12. Consider Public Memory Provenance Header Only After Route3 And Lowering Consumers Are Mapped

Owned files:

- `src/backend/bir/bir.hpp`
- possible new `src/backend/bir/bir_memory_provenance.hpp`
- existing route3 implementation/header files
- relevant `src/backend/bir/lir_to_bir/` include sites only when compile proof
  requires include updates

Non-goals:

- Do not move memory provenance declarations into `lir_to_bir/memory/`.
- Do not change pointer-value provenance, static GEP authority, object extent,
  byte-range, or dynamic-array semantics.
- Do not edit idea 422 producer behavior.
- Do not move core `MemoryAddress` payload usage unless a separate core-header
  split owns that work.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must cover route3 memory access, LIR-to-BIR memory
  lowering consumers, object emission, and pointer-value provenance tests.
- The supervisor should prefer broader backend validation because this touches
  a public model support surface with many consumers.

Reviewer reject signals:

- Public model consumers become dependent on private lowering headers.
- Any provenance authority verdict changes.
- The slice combines declaration movement with new memory behavior.
- `Function` or instruction storage layout changes as an incidental side
  effect.

Why behavior-preserving:

- A later memory provenance header can reduce `bir.hpp` size only by moving
  existing declarations and include edges. It must preserve the same public
  records, enum values, storage layout, and route3/lowering consumer behavior.

### 13. Consider Local-Array And Semantic-GEP Header Split After Memory Ownership Is Stable

Owned files:

- `src/backend/bir/bir.hpp`
- possible new local-array or semantic-GEP analysis header
- include sites required by compile proof
- focused tests for local array, semantic GEP, scalar local load, and static
  GEP authority consumers

Non-goals:

- Do not move implementation behavior in the same slice.
- Do not change `Function` storage semantics.
- Do not change memory provenance authority policy.
- Do not fold this work into LIR-to-BIR producer capability.

Validation expectations:

- Build proof: `cmake --build --preset default`.
- Focused backend proof must cover local-array proof records, semantic GEP,
  scalar local loads, and any static GEP authority consumers.
- Include a broad backend subset if `Function` storage declarations are touched.

Reviewer reject signals:

- Header movement changes record layout, vector storage, optionality, or lookup
  behavior.
- New include cycles appear between core model, memory provenance, and route
  headers.
- The slice edits lowering behavior or capability tests to justify the split.
- It proceeds before memory provenance ownership is settled.

Why behavior-preserving:

- This is a declaration organization idea only. It can preserve behavior by
  keeping the same records and storage members while shifting them behind a
  narrower public analysis header after memory ownership is no longer
  ambiguous.

## Deferred Or Rejected As Follow-Up Ideas

- Idea 422 producer implementation: separate capability work, not file
  organization cleanup.
- Broad `Value`, `Inst`, `Block`, `Function`, `Module`, and core instruction
  model movement: late/no-move until route and analysis families are extracted.
- Stateful `BirFunctionLowerer` memory/provenance method movement: possible
  future private lowering cleanup, but it is not a public BIR core cleanup
  follow-up from this destination map.
- One-shot `bir.hpp` breakup: too broad and too likely to mix declaration
  churn, include churn, and behavior changes.
- A new monolithic `bir_routes.hpp`: likely recreates the current coupling
  under a different filename unless narrower route headers are proven.
