# Legacy Capability Coverage Ledger

Status: reviewed disposition ledger for the current `src/backend/legacy/`
inventory. This is migration evidence, not permission to compile, restore, or
extend legacy code. Implementation of the replacement architecture remains
incomplete.

`Accepted` means the semantic behavior has exactly one named new owner.
`Reject` means the old representation or alternate route is deliberately not
retained. `Defer` means the capability belongs to a named later non-BIR owner
or remains an explicit source/implementation gap; no earlier stage may infer
it. A legacy file may supply test vectors and audit evidence after its
authority is rejected.

## Mechanically checked path ledger

The first column of every row is one Python regular expression relative to
`src/backend/legacy/`. The source uses Markdown's `\|` table escape for regex
alternation; the coverage checker extracts only first-column code spans,
removes that presentation escape, and applies `re.fullmatch`. Therefore every
current file must match exactly one row and every expression must match at
least one current file.
Alternation is grouped inside a single row only when every matched file has the
same disposition and owner. A row containing `Accepted` contains exactly one
literal `Producer/owner:`. Reject and Defer rows name their enforcing
`Boundary:` instead.

| Full-match legacy-path regex | Disposition | Accepted owner or boundary |
|---|---|---|
| `(?:bir\.(?:cpp\|hpp)\|bir_private\.hpp\|query\.(?:cpp\|hpp))` | Accepted core identity and traversal; reject pointer/name/index identity and duplicate storage | Producer/owner: indexed `core/README.md`; `verify/README.md` is the validity consumer/gate. |
| `bir_validate\.cpp` | Accepted typed structural and semantic verification; reject boolean/free-form validation and spelling fallback | Producer/owner: indexed `verify/README.md` profile rule registries. |
| `(?:lir_to_bir\.(?:cpp\|hpp)\|lir_adapter_error\.hpp)` | Accepted typed import and error transport; reject text recovery and partial publication | Producer/owner: indexed `lir_to_bir/README.md`; missing structured source carriers remain explicit source failures. |
| `(?:bir_route[1-8](?:_[a-z_]+)?\.cpp\|bir_route_facade\.cpp\|bir_route_index(?:_prereqs)?\.hpp)` | Accepted underlying typed semantic facts; reject every route index, facade, selected path, and agreement record | Producer/owner: indexed `core/README.md` typed instructions and exact def-use; analyses may derive disposable exact-revision results. |
| `bir_producer_view\.hpp` | Accepted direct definition/producer queries; reject same-block positions, names, pointers, and a persistent producer index | Producer/owner: indexed `core/README.md` exact def-use and typed traversal. |
| `(?:bir_select_dependency_view\|bir_comparison_view)\.(?:cpp\|hpp)` | Accepted comparison, condition-producer, and select-chain queries; reject selected-path snapshots | Producer/owner: indexed `analysis/comparison/README.md`. |
| `bir_memory_access_view\.hpp` | Accepted typed access/effect queries; reject legacy access records and stored access truth | Producer/owner: indexed `analysis/memory_effects/README.md`. |
| `bir_memory_provenance\.hpp` | Accepted conservative object-origin, path, and range queries; reject spelling identity and persistent proof records | Producer/owner: indexed `analysis/provenance/README.md`. |
| `(?:bir_publication_view\.hpp\|bir_call_boundary_view\.(?:cpp\|hpp)\|bir_return_view\.(?:cpp\|hpp))` | Accepted semantic value-flow, call-boundary, and return-position queries; reject publication routes and ABI lanes | Producer/owner: indexed `analysis/publication/README.md`. |
| `bir_control_flow_view\.(?:cpp\|hpp)` | Reject the combined view and relationship snapshots | Boundary: typed terminators/phi edges, indexed CFG, comparison, and publication contracts retain disjoint authority; no combined producer exists. |
| `bir_local_array_semantic_gep\.hpp` | Accepted semantic address behavior; reject testcase-shaped local-array routing | Producer/owner: indexed `passes/memory/README.md`; C6 consumes the normalized address. |
| `(?:bir_printer\.cpp\|bir_render\.cpp)` | Accepted presentation only; reject rendered identity or compiler feedback | Producer/owner: indexed `diagnostics/README.md`. |
| `prealloc/(?:module\.hpp\|storage\.hpp\|value_locations\.hpp\|names\.hpp\|label_identity\.(?:cpp\|hpp))` | Reject the monolithic prepared graph, physical-home side tables, spelling identity, and label fallback | Boundary: typed preparation products borrow Canonical BIR; pseudo graph revisions, E2 homes, and diagnostics have separate indexed owners. |
| `prealloc/(?:prepared_lookups\|lookup_agreement\|prepared_object_traversal\|select_chain_lookups)\.(?:cpp\|hpp)` | Accepted ordinary typed traversal; reject duplicate lookup-agreement and persistent indices | Producer/owner: indexed `core/README.md`; product-local and analysis queries remain revision-bound consumers. |
| `prealloc/(?:prepared_contract_verifier\.(?:cpp\|hpp)\|prepared_fact_boundary\.hpp)` | Accepted typed invariants; reject one omnibus prepared-state verifier and untyped fact boundary | Producer/owner: indexed `verify/README.md` profile rule registries. |
| `prealloc/prepared_printer\.(?:cpp\|hpp)` | Accepted read-only presentation; reject printer-driven phase truth | Producer/owner: indexed `diagnostics/README.md`. |
| `prealloc/prepared_printer/.*` | Accepted nested read-only presentation for addressing, calls, control flow, storage, allocation, and other prepared evidence; reject every printer as an authority | Producer/owner: indexed `diagnostics/README.md`; renderers borrow typed views only. |
| `prealloc/(?:legalize\.cpp\|comparison\.(?:cpp\|hpp)\|control_flow\.hpp\|atomics\.(?:cpp\|hpp))` | Accepted target-independent normalization semantics; reject prepared special cases | Producer/owner: indexed canonical `passes/README.md` and its subordinate contracts; D4 and F1 only consume the normalized result. |
| `prealloc/(?:addressing\.hpp\|pointer_value_memory_freshness\.hpp)` | Accepted conservative semantic origin/path queries; reject stored freshness truth, standalone alias authority, and concrete address modes | Producer/owner: indexed `analysis/provenance/README.md`; C6 address preparation consumes the exact-revision result. |
| `prealloc/(?:calls\.hpp\|call_plans\.(?:cpp\|hpp)\|formal_publications\.(?:cpp\|hpp)\|publication_plans\.(?:cpp\|hpp))` | Accepted ABI/call transport semantics; reject publication routes and hidden operands | Producer/owner: indexed D2 `passes/call_lowering/README.md`; C3/C4 and publication analysis supply verified inputs. |
| `prealloc/(?:variadic\.hpp\|variadic_entry_plans\.(?:cpp\|hpp))` | Accepted variadic planning | Producer/owner: indexed C5 `preparation/variadic/README.md`; D2 consumes the plan. |
| `prealloc/inline_asm\.(?:cpp\|hpp)` | Reject duplicate constraint interpretation, special carriers, and early asm parsing | Boundary: indexed C7 preparation owns vocabulary, C9 owns binding/projection, and F3 alone interprets opaque template text for assembly. |
| `prealloc/(?:intrinsics\.(?:cpp\|hpp)\|runtime_helpers\.hpp\|i128_runtime_helpers\.(?:cpp\|hpp)\|f128_runtime_helpers\.(?:cpp\|hpp))` | Accepted intrinsic normalization; reject helper choice in Canonical BIR | Producer/owner: indexed `passes/intrinsics/README.md`; C8, D1, and D2 consume normalized semantics. |
| `prealloc/target_register_profile\.(?:cpp\|hpp)` | Accepted finite abstract target-pool descriptors; reject concrete-register facts in BIR nodes | Producer/owner: indexed C2 `target_layout/README.md`; F1 consumes verified one-to-one mappings. |
| `prealloc/liveness\.(?:cpp\|hpp)` | Accepted allocation liveness/interference semantics; reject reuse across a revision change | Producer/owner: indexed E1 `analysis/liveness/README.md`. |
| `prealloc/out_of_ssa\.cpp` | Accepted phi elimination and copy-resolution semantics; reject residual phi or target-register copies | Producer/owner: indexed D5 `passes/out_of_ssa/README.md`. |
| `prealloc/(?:decoded_home_storage\|special_carriers)\.(?:cpp\|hpp)` | Accepted only fixed abstract ABI roles representable as ordinary typed BIR; reject decoded-home mirrors and special carriers | Producer/owner: indexed D2 `passes/call_lowering/README.md`; E2 later assigns ordinary abstract homes. |
| `prealloc/storage_plans\.(?:cpp\|hpp)` | Accepted abstract spill-state planning; reject concrete frame locations and hidden transitions | Producer/owner: indexed E3 `regalloc/spill_reload/README.md`, using abstract spill objects and explicit Spill/Reload nodes. |
| `prealloc/(?:dynamic_stack\.(?:cpp\|hpp)\|frame\.hpp)` | Accepted semantic dynamic-stack operations; reject concrete placement in core or Canonical BIR | Producer/owner: indexed `core/README.md`; E4 later owns exact frame placement through `FrameRealizationTransaction`. |
| `prealloc/frame_plan\.(?:cpp\|hpp)` | Accepted exact frame-placement capability; reject the legacy plan representation and any second planner | Producer/owner: indexed E4 `allocated/README.md` `FrameRealizationTransaction`; F1 is apply-only. |
| `prealloc/object_data\.(?:cpp\|hpp)` | Defer object bytes and relocations | Boundary: indexed `../mir/object/README.md`; Canonical BIR retains typed initializer and symbol semantics only. |
| `prealloc/(?:prealloc\.(?:cpp\|hpp)\|README\.md)` | Reject the monolithic driver, phase flags, completed-phase strings, and alternate publication route | Boundary: the root indexed A1-F3 pipeline and its named stage owners. |
| `prealloc/regalloc\.(?:cpp\|hpp)` | Reject the monolithic allocation coordinator, mixed physical-location record, and MIR repair route | Boundary: indexed E1, E2, E3, D5, and E4 contracts divide analysis, assignment, spill state, copy resolution, and publication. |
| `prealloc/regalloc_placement_identity\.(?:cpp\|hpp)` | Accepted abstract assignment identity semantics; reject legacy physical-name identity and named-case placement | Producer/owner: indexed E2 `regalloc/README.md`. |
| `prealloc/regalloc/intervals\.(?:cpp\|hpp)` | Accepted interval and interference semantics; reject unkeyed or stale analysis | Producer/owner: indexed E1 `analysis/liveness/README.md`. |
| `prealloc/regalloc/(?:assignment\|classification\|storage\|value_homes\|values)\.(?:cpp\|hpp)` | Accepted abstract-home classification and assignment semantics; reject physical spellings and stack-offset assignment | Producer/owner: indexed E2 `regalloc/README.md`. |
| `prealloc/regalloc/(?:spill_reload\|stack_slots)\.(?:cpp\|hpp)` | Accepted abstract spill objects and explicit spill/reload semantics; reject concrete offsets and hidden spill repair | Producer/owner: indexed E3 `regalloc/spill_reload/README.md`. |
| `prealloc/regalloc/(?:call_moves\|call_return_abi\|runtime_helpers)\.(?:cpp\|hpp)` | Accepted ABI/helper transport requirements; reject late side-record moves and register-name recovery | Producer/owner: indexed D2 `passes/call_lowering/README.md`, consuming C3/C4/C5/C8 facts. |
| `prealloc/regalloc/(?:consumer_moves\|move_records\|phi_moves)\.(?:cpp\|hpp)` | Accepted phi/parallel-copy behavior; reject post-allocation hidden move records | Producer/owner: indexed D5 `passes/out_of_ssa/README.md` copy-resolution closure. |
| `prealloc/regalloc/pointer_carriers\.(?:cpp\|hpp)` | Reject prepared pointer-carrier mirrors, cycle/name heuristics, and frame-address authority | Boundary: indexed provenance plus C6 preparation preserve semantic address requirements; D4 expands target forms and E4 fixes frame mappings. |
| `prealloc/stack_layout/(?:analysis\|alloca_coalescing\|regalloc_helpers\|slot_assignment)\.cpp` | Accepted exact object/frame placement capability; reject legacy offset assignment and post-allocation repair | Producer/owner: indexed E4 `allocated/README.md` `FrameRealizationTransaction`, consuming exact E3 spill state and fixed call/frame requirements. |
| `prealloc/stack_layout/copy_coalescing\.cpp` | Reject a second late copy/coalescing authority | Boundary: indexed D5 owns allocation-aware copy resolution before E4; E4 cannot rewrite copy semantics. |
| `prealloc/stack_layout/inline_asm\.cpp` | Reject special-case late inline-asm placement | Boundary: C9 projection, E2 assignment, D5 resolution, and E4 frame realization supply complete facts; F1 only applies them. |
| `prealloc/stack_layout/(?:README\.md\|coordinator\.cpp\|lookups\.cpp\|stack_layout\.hpp)` | Reject the duplicate stack-layout coordinator, cache, lookup authority, and alternate publication route | Boundary: indexed E4 alone owns atomic `FrameRealizationTransaction`; F1 cannot plan or repair layout. |

## Coverage gate

Architecture review runs the documented exact-match checker over every current
file under `src/backend/legacy/`. A new file fails until it receives one and
only one disposition. A new expression also fails if it matches no checked-in
file, preventing stale or aspirational coverage rows.

Every `Accepted` row has one present indexed producer/owner and may name
multiple read-only consumers. A Defer row remains forbidden at every earlier
profile. A Reject row must not be recreated under a new name or hidden in a
cache, renderer, adapter, target hook, or test helper. In particular:

- E1 owns liveness/interference, E2 owns abstract assignment, E3 owns abstract
  spill state, D2 owns call transport, and D5 owns phi/copy realization.
- C5 owns variadic planning only. Provenance supplies semantic origin facts and
  C6 owns address preparation.
- E4 alone produces the immutable exact-revision `FrameRealizationPlan` through
  `FrameRealizationTransaction`; F1 applies that plan one-to-one and cannot
  select offsets, coordinate layout, expand records, allocate, or repair.

Before implementation acceptance, symbol-level review must record exact
inputs/outputs, failure behavior, target differences, adjacent same-feature
tests, and deletion of duplicated routes. Coverage is incomplete if proof
demonstrates only a named testcase, rewrites an expectation, or leaves a nearby
member of the same feature family unexamined.
