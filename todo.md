Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Document Retained Compatibility Bridges

# Current Packet

## Just Finished

Step 8 documented the retained raw-name compatibility bridges after the Step
6/7 implementation and proof through commit `2a7af71f3` (`Prove BIR
initializer LinkNameId validation`). Remaining raw string paths classify as:

- Output/diagnostics: BIR validation and backend-preparation errors still print
  function/global/value names as text. Owner: reporting sites. Limitation: text
  is only user-facing context and must not reopen lookup when a valid
  `LinkNameId` is present. Removal condition: none for diagnostics/output.
- Route-local names: BIR SSA values, params/results, block labels, local slots,
  prepared function-name ids, register/slot/addressing handles, string-pool
  labels, and type spellings remain strings. Owner: BIR and prealloc
  route-local storage flows. Limitation: these names identify local compiler
  artifacts, not module-level link-visible symbols. Removal condition: only a
  separate route-local-id cleanup, not idea 162.
- BIR validation compatibility: `find_global`/`find_function` and initializer
  symbol validation use raw names only when the instruction/global has
  `kInvalidLinkName`. Owner: `src/backend/bir/bir_validate.cpp`. Limitation:
  valid `LinkNameId` first resolves by id and mismatch checks reject
  conflicting declared raw names. Removal condition: legacy/raw-only BIR inputs
  are retired or all covered BIR producers always provide complete
  `LinkNameId`.
- Backend direct-call compatibility: `resolve_direct_callee` uses raw
  `call.callee` only when `callee_link_name_id` is invalid. Owner:
  `src/backend/prealloc/prealloc.cpp`. Limitation: valid direct call
  `LinkNameId` resolves through the module name table and refuses mismatched
  raw spelling. Removal condition: raw-only direct-call BIR compatibility is no
  longer supported.
- Backend symbol-pointer compatibility: `resolve_symbol_pointer_name` and call
  argument preparation preserve raw `@symbol` only for pointer values with
  invalid `pointer_symbol_link_name_id`. Owner:
  `src/backend/prealloc/prealloc.cpp`. Limitation: valid pointer symbol ids
  must resolve to a declared module symbol and cannot conflict with retained
  display text. Removal condition: all symbol pointer values entering backend
  preparation carry valid `LinkNameId`.
- Backend direct-address compatibility: `build_direct_symbol_backed_address`
  interns the raw fallback symbol only when no address/base `LinkNameId` is
  available. Owner:
  `src/backend/prealloc/stack_layout/coordinator.cpp`. Limitation: valid
  address ids resolve from BIR link names and reject mismatched fallback/base
  spelling. Removal condition: raw-only global load/store/address BIR
  compatibility is removed.
- LIR-to-BIR unresolved-boundary maps: `GlobalTypes` remains keyed by producer
  final spelling, and `FunctionSymbolSet::raw_symbol_link_name_ids` maps raw
  LIR symbol operands back to known function ids. Owner:
  `src/backend/bir/lir_to_bir/*`. Limitation: these are LIR-boundary bridge
  tables; lowered BIR instructions and symbol pointer values carry `LinkNameId`
  when available. Removal condition: LIR operands and initializer metadata
  expose structured symbol ids directly, so no raw `@name` bridge table is
  needed.
- LIR-to-BIR initializer compatibility:
  `resolve_initializer_symbol_link_name_id`, pointer-initializer offset
  resolution, provenance lowering, and local-slot pointer stores still parse
  raw `@symbol` operands when the source metadata has no valid id. Owner:
  `src/backend/bir/lir_to_bir/module.cpp` and
  `src/backend/bir/lir_to_bir/memory/*`. Limitation: known functions/globals
  are immediately reattached to `LinkNameId`; a provided but unknown
  initializer-function id fails closed instead of falling back to raw spelling.
  Removal condition: all aggregate/byval/pointer initializer routes arrive from
  LIR with complete structured symbol metadata.

Closure classification: idea 162 is ready for closure review from this
executor's packet. I did not identify a concrete remaining implementation gap
inside the owned Step 8 scope; the retained raw paths above are output,
diagnostics, route-local names, or documented compatibility/no-metadata
bridges.

## Suggested Next

Supervisor should request closure review for idea 162, using the Step 8 bridge
classification above plus the existing Step 6/7 proof notes. No executor
implementation packet is recommended before review.

## Watchouts

- No production validator change was needed; existing global load/store
  validation and initializer-symbol validation already use `LinkNameId` as
  authoritative when present.
- Raw same-module global names and initializer-symbol spellings remain accepted
  only for compatibility contracts where the BIR reference has an invalid
  `LinkNameId`.
- The LIR-to-BIR raw `@symbol` bridge tables are the main retained
  compatibility surface; reviewer should decide whether their documented
  no-metadata limitation is acceptable for closure or should become a new,
  separate LIR operand metadata idea.
- The untracked `review/` artifacts were not touched.

## Proof

Docs/todo-only change; no build or test was required by the delegated proof
contract. `test_after.log` was left untouched from the prior proof.
