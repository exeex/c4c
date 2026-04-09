# Active Todo

Status: Active
Source Idea: ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md
Source Plan: plan.md

## Active Item

- Step 7: move regalloc and stack layout to backend MIR
- Current slice: retarget any remaining non-emitter production callers that
  still go through `prepare_module_function_entry_alloca_inputs(...)` even
  though they only need rewrite or planning state
- Current implementation target: audit whether any non-emitter production paths
  besides `rewrite_module_entry_allocas(...)` still force compatibility
  `StackLayoutInput` rehydration instead of consuming the narrower prepared
  rewrite/planning seams directly
- Next intended slice: if no additional non-emitter production callers remain,
  narrow the compatibility wrapper naming and comments so emitter-only
  ownership is explicit at the public `stack_layout` entrypoints

## Completed

- Completed the next Step 7 rewrite-only preparation split by letting module
  entry-alloca rewriting consume the narrowed prepared seams without
  rehydrating the compatibility `StackLayoutInput`:
  - added `PreparedEntryAllocaRewriteOnlyInputs` plus
    `lower_prepared_entry_alloca_rewrite_only_inputs(...)` /
    `prepare_module_function_entry_alloca_rewrite_only_inputs(...)` in
    `src/backend/stack_layout/slot_assignment.hpp` and
    `src/backend/stack_layout/slot_assignment.cpp`
  - retargeted `rewrite_module_entry_allocas(...)` to the new rewrite-only
    prepared-input path so the module rewrite flow now consumes only
    `liveness_input` plus the narrowed rewrite/planning state, leaving the
    full compatibility `StackLayoutInput` rehydration to callers that still
    need slot-building metadata
  - kept `prepare_module_function_entry_alloca_inputs(...)` as the compatibility
    wrapper used by the AArch64 direct-LIR emitter and tests that explicitly
    exercise the rehydrated stack-layout view
  - added focused coverage in `tests/backend/backend_shared_util_tests.cpp`
    proving the rewrite-only prepared-input path preserves the same narrowed
    rewrite/planning/liveness state for both lowerable and fallback functions
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo's allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 7 lowerable-function stack-layout-source narrowing
  slice by removing the remaining prepared per-function
  `RawLirFunction` source from the lowerable BIR-backed branch:
  - updated `src/backend/stack_layout/slot_assignment.cpp` so the
    lowerable-function branch in
    `prepare_module_function_entry_alloca_preparation(...)` now derives
    entry-alloca classification and metadata through
    `lower_function_entry_alloca_stack_layout_input(function, backend_cfg)`
    instead of calling `lower_lir_to_stack_layout_input(function)`
  - switched that lowerable prepared carrier to report
    `EntryAllocasAndBackendCfg` as its stack-layout source so lowerable and
    fallback preparation now agree on the backend-owned metadata seam while
    still keeping per-function BIR liveness as the production liveness source
  - updated `tests/backend/backend_shared_util_tests.cpp` to lock the new
    source expectation for both prepared preparation and the compatibility
    wrapper
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`

- Completed the first Step 7 stack-layout entrypoint-splitting slice by marking
  compatibility-only raw-`LIR` surfaces explicitly and retargeting the
  highest-value production caller onto prepared per-function inputs:
  - marked `src/backend/stack_layout/analysis.hpp`'s direct raw-`LIR`
    stack-layout lowerers as compatibility-only surfaces and documented the
    backend-owned prepared per-function seam as the preferred production path
  - marked the `StackLayoutInput` overloads in
    `src/backend/stack_layout/slot_assignment.hpp` as compatibility wrappers
    around the narrower planning/rewrite seams so the public ownership split is
    explicit in the shared header
  - updated `src/backend/aarch64/codegen/emit.cpp` so the general AArch64
    direct-LIR emitter now gets stack-layout metadata through
    `prepare_module_function_entry_alloca_inputs(...)` instead of calling
    `lower_lir_to_stack_layout_input(...)` directly during unsupported-feature
    gating and stack-slot setup
  - added focused coverage in `tests/backend/backend_shared_util_tests.cpp`
    proving prepared per-function stack-layout inputs preserve the slot-building
    value set and signature/call metadata the emitter needs for both lowerable
    and fallback functions
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo's allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Closed Step 6 as complete enough for plan execution:
  - production liveness/regalloc now center on
    `backend CFG -> LivenessInput -> compute_live_intervals(...)`
  - the remaining direct raw-`LIR` liveness entrypoint is explicitly marked
    compatibility-only in `src/backend/liveness.hpp`
  - follow-up Step 6 narrowing in `stack_layout` clarified planning and
    rewrite seams enough that Step 7 can proceed without reopening liveness
    ownership

## Notes

- The largest remaining raw-`LIR` ownership seam is now in
  `src/backend/stack_layout/analysis.*` and
  `src/backend/stack_layout/slot_assignment.*`, not in `liveness` /
  `regalloc`.
- Worker packet files from the previous subagent round have been cleared so the
  repo returns to one canonical `todo.md`.

- Completed the next Step 6 `EntryAllocaInput` payload narrowing slice by
  splitting slot-planning metadata away from the rewrite-owned literal
  paired-store payload:
  - added `EntryAllocaPairedStorePlanInfo`, `EntryAllocaPlanInput`, and
    `EntryAllocaPlanningInput` in
    `src/backend/stack_layout/slot_assignment.hpp` so entry-alloca slot
    planning now consumes only alloca identity/type/alignment plus paired-store
    classification (`has_store`, `is_zero_initializer`, optional param name)
    and the coarse escape/use-block seams needed for coalescing
  - updated `src/backend/stack_layout/slot_assignment.cpp` so
    `plan_entry_alloca_slots(...)` lowers the rewrite-owned
    `EntryAllocaRewriteInput` into the new planning-only contract before
    assigning slots, leaving `build_entry_alloca_rewrite_patch(...)` as the
    sole owner of the literal `paired_store_value` needed to re-emit entry
    stores
  - recorded the new invariant that `alloca_name` / `type_str` / `align` plus
    paired-store classification are sufficient for planning, while only patch
    synthesis still requires the full literal paired-store operand
  - updated `tests/backend/backend_shared_util_tests.cpp` with focused coverage
    proving the narrowed planning surface still handles zero-init overwrite
    pruning and dead param-alloca pruning without a full rewrite record
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`

- Completed the next Step 6 prepared rewrite-contract narrowing slice by
  introducing a dedicated rewrite-owned `EntryAllocaRewriteInput` and retargeting
  production entry-alloca rewrite prep away from the broader
  `StackLayoutInput` contract:
  - added `EntryAllocaRewriteInput` in
    `src/backend/stack_layout/slot_assignment.hpp` so production rewrite prep
    now carries only `entry_allocas` plus the coarse
    `escaped_entry_allocas` / `entry_alloca_use_blocks` /
    `entry_alloca_first_accesses` seams instead of the full stack-layout
    surface
  - updated `src/backend/stack_layout/slot_assignment.cpp` so
    `prepare_entry_alloca_rewrite_patch(...)`, prepared entry-alloca lowering,
    and the production module rewrite path now plan and synthesize patches from
    the new narrow rewrite input while keeping the full `StackLayoutInput`
    rehydration only as compatibility state for existing callers and tests
  - recorded the current invariant that `plan_entry_alloca_slots(...)` plus
    `build_entry_alloca_rewrite_patch(...)` still jointly require
    `EntryAllocaInput`'s `alloca_name`, `type_str`, `align`, and
    `paired_store_value`, so the contract narrowed at the container boundary
    rather than by deleting any of those fields yet
  - updated `tests/backend/backend_shared_util_tests.cpp` with focused coverage
    proving prepared rewrite inputs now expose the narrow rewrite contract and
    that the production patch-prep path accepts that narrower input directly
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 prepared fallback `entry_allocas` ownership
  narrowing slice by removing entry-alloca records from the backend-CFG-derived
  stack-layout classification carrier and moving them onto a dedicated
  rewrite-owned prepared seam:
  - removed `entry_allocas` from
    `src/backend/stack_layout/slot_assignment.hpp`'s
    `PreparedEntryAllocaStackLayoutClassificationInput`
  - added `PreparedEntryAllocaRewriteMetadata` in
    `src/backend/stack_layout/slot_assignment.hpp` so prepared entry-alloca
    records remain explicitly owned by the rewrite path instead of being mixed
    into backend-CFG classification state
  - updated `src/backend/stack_layout/slot_assignment.cpp` so preparation now
    stores entry-alloca records on the new rewrite-owned seam while lowering
    still rehydrates production `StackLayoutInput::entry_allocas` before slot
    planning and rewrite patch synthesis
  - updated `tests/backend/backend_shared_util_tests.cpp` to prove prepared
    entry-alloca rewrite prep now exposes those records through rewrite
    metadata while the classification carrier remains limited to backend-CFG
    stack-layout facts
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_before.log`; full suite summary `2841` total with `32` failing both
    before and after)

- Completed the next Step 6 prepared block-arity narrowing slice by removing
  duplicate prepared per-block instruction counts from the stack-layout
  fallback carrier and rehydrating block structure entirely from backend-owned
  liveness:
  - removed `PreparedEntryAllocaStackLayoutBlock` and the
    `PreparedEntryAllocaStackLayoutClassificationInput::blocks` cache from
    `src/backend/stack_layout/slot_assignment.hpp`
  - updated `src/backend/stack_layout/slot_assignment.cpp` so prepared
    stack-layout lowering now rebuilds block count, block labels, and
    per-block instruction arity directly from `LivenessInput` before
    rehydrating use lists, rather than caching duplicate prepared block counts
  - updated `tests/backend/backend_shared_util_tests.cpp` to prove prepared
    fallback lowering now depends on backend-owned liveness for block
    instruction counts and no longer exposes cached prepared block-count state
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 prepared phi-edge narrowing slice by removing
  duplicate prepared phi predecessor-use storage from the stack-layout fallback
  carrier and rehydrating that data from backend-owned liveness during
  lowering:
  - removed `phi_incoming_uses` from
    `src/backend/stack_layout/slot_assignment.hpp`'s
    `PreparedEntryAllocaStackLayoutClassificationInput`
  - updated `src/backend/stack_layout/slot_assignment.cpp` so prepared
    stack-layout lowering now rebuilds `StackLayoutInput::phi_incoming_uses`
    from `LivenessInput::phi_incoming_uses` instead of caching a duplicate
    prepared copy
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving prepared fallback lowering still restores phi
    predecessor-edge uses from backend-owned liveness while preserving the
    correct predecessor block mapping for stack-layout analysis
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 prepared block-label narrowing slice by removing
  cached prepared block labels and rehydrating block identity from backend-owned
  liveness order:
  - removed `PreparedEntryAllocaStackLayoutBlock::label` from
    `src/backend/stack_layout/slot_assignment.hpp`
  - updated `src/backend/stack_layout/slot_assignment.cpp` so prepared
    stack-layout lowering now stores only per-block instruction counts and
    reconstructs `StackLayoutInput` block labels from the already-owned
    liveness carrier by block index
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving prepared fallback lowering still restores block labels
    from liveness and preserves phi predecessor-edge block attribution after
    the prepared label cache is removed
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 prepared fallback carrier arity narrowing slice by
  replacing empty per-instruction prepared point shells with block-local
  instruction counts:
  - removed the unused
    `PreparedEntryAllocaStackLayoutPoint` type from
    `src/backend/stack_layout/slot_assignment.hpp` and collapsed
    `PreparedEntryAllocaStackLayoutBlock` to `{ label, inst_count }`
  - updated `src/backend/stack_layout/slot_assignment.cpp` so prepared
    stack-layout classification stores only block labels plus instruction arity
    and production `StackLayoutInput` rehydration rebuilds empty points from
    `inst_count` before repopulating use lists from liveness
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the prepared fallback carrier now tracks only block-local
    instruction counts while lowering still reconstructs the full per-
    instruction stack-layout use lists from the liveness seam
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 prepared pointer-access narrowing slice by
  dropping cached per-point access facts from the prepared fallback
  stack-layout carrier once coarse backend-owned first-access, use-block, and
  escape seams covered the remaining consumers:
  - removed stored `pointer_accesses` payloads from
    `PreparedEntryAllocaStackLayoutPoint` in
    `src/backend/stack_layout/slot_assignment.hpp`
  - updated `src/backend/stack_layout/slot_assignment.cpp` so prepared
    stack-layout classification now keeps only block labels plus instruction
    arity for rehydration while the lowered production `StackLayoutInput`
    leaves per-point `pointer_accesses` empty
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving overwrite-before-read pruning still trusts the coarse
    `entry_alloca_first_accesses` seam after prepared pointer-access facts are
    removed entirely, and still regresses once that coarse seam is also cleared
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as `test_before.log`)

- Completed the next Step 6 prepared derived-pointer-root narrowing slice by
  removing cached per-point alias metadata from the prepared fallback
  stack-layout carrier once the coarse backend-owned seams covered the
  remaining consumers:
  - removed `derived_pointer_root` from
    `PreparedEntryAllocaStackLayoutPoint` in
    `src/backend/stack_layout/slot_assignment.hpp`
  - updated `src/backend/stack_layout/slot_assignment.cpp` so prepared
    classification lowering no longer stores per-point derived-pointer-root
    aliases and lowering back into the production `StackLayoutInput` no longer
    rehydrates them
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving prepared fallback lowering still preserves single-block
    coalescing through `entry_alloca_use_blocks` while the lowered
    `StackLayoutInput` no longer carries prepared derived-pointer-root facts
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`

- Completed the next Step 6 prepared overwrite-before-read classification
  narrowing slice by moving fallback first-access ownership onto a coarse
  entry-alloca seam:
  - added optional `entry_alloca_first_accesses` storage to
    `src/backend/stack_layout/analysis.hpp` and to the prepared fallback
    classification carrier in `src/backend/stack_layout/slot_assignment.hpp`
  - taught `src/backend/stack_layout/slot_assignment.cpp` to derive that
    coarse first-access classification during prepared fallback lowering and to
    round-trip it back into the production `StackLayoutInput`
  - updated `src/backend/stack_layout/analysis.cpp` so overwrite-before-read
    analysis now trusts the coarse `entry_alloca_first_accesses` seam when
    present instead of rebuilding only from per-point `pointer_accesses`
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving stack-layout analysis accepts the coarse first-access seam
    directly and that prepared fallback lowering preserves overwrite-before-
    read pruning after clearing prepared pointer-access facts while still
    regressing once both the new seam and the old per-point facts are removed
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 prepared single-block coalescing narrowing slice by
  moving fallback coalescing ownership onto a coarse entry-alloca use-blocks
  seam:
  - added `EntryAllocaUseBlocks` to `src/backend/stack_layout/analysis.hpp`
    plus optional `entry_alloca_use_blocks` storage on both
    `StackLayoutInput` and the prepared fallback
    `PreparedEntryAllocaStackLayoutClassificationInput` carrier in
    `src/backend/stack_layout/slot_assignment.hpp`
  - taught `src/backend/stack_layout/slot_assignment.cpp` to derive that coarse
    per-entry-alloca use-block classification during prepared fallback
    lowering and to round-trip it back into the production stack-layout input
  - updated `src/backend/stack_layout/alloca_coalescing.cpp` so single-block
    reuse classification now trusts the coarse entry-alloca use-block seam when
    present instead of rebuilding solely from cached per-point
    `derived_pointer_root` and `pointer_accesses`
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving alloca coalescing accepts the coarse use-block seam and
    that prepared fallback lowering keeps single-block classification alive
    after clearing derived-pointer-root facts while still regressing once both
    the new seam and the old per-point facts are removed
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 prepared escaped-alloca classification narrowing
  slice by moving fallback escape ownership off cached per-point
  `escaped_names` and onto a narrower coarse escape set:
  - added `escaped_entry_allocas` to the prepared fallback classification
    carrier in `src/backend/stack_layout/slot_assignment.*` and taught
    preparation/lowering to derive and round-trip that coarse entry-alloca
    escape set while dropping prepared per-point `escaped_names`
  - updated `src/backend/stack_layout/analysis.hpp` and
    `src/backend/stack_layout/alloca_coalescing.cpp` so
    `StackLayoutInput` can carry the optional precomputed escape set and
    coalescing trusts that narrower seam when present instead of requiring
    rebuilt per-point escaped-name lists
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the prepared fallback carrier now stores escaped alloca
    classification as a coarse set and that clearing that set still regresses
    escaped-alloca exclusion on the prepared backend-CFG path
  - rebuilt `backend_shared_util_tests` and passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
  - rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 prepared pointer/escape audit slice by proving the
  current fallback carrier cannot shrink beyond the already-removed
  `used_names` duplication without a new backend-owned classification seam:
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    regressions that clear prepared `pointer_accesses`,
    `derived_pointer_root`, and `escaped_names` one at a time and verify each
    removal regresses one of the still-required fallback contracts
    (overwrite-before-read pruning, aggregate single-block coalescing, and
    escaped-alloca exclusion)
  - recorded the audit outcome that no further prepared pointer/escape field is
    removable on today’s backend-CFG/liveness seam, so the next Step 6 slice
    needs a narrower backend-owned stack-layout classification carrier rather
    than another blind field deletion
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded prepared stack-layout use-list narrowing
  slice by removing duplicated `used_names` storage from the prepared
  classification carrier and rebuilding those use lists from liveness during
  lowering:
  - removed instruction-level `used_names` and block-level
    `terminator_used_names` from
    `src/backend/stack_layout/slot_assignment.*`'s prepared
    `PreparedEntryAllocaStackLayoutClassificationInput` carrier so the cached
    fallback classification now keeps only the pointer/escape facts still owned
    by overwrite-before-read and alloca-coalescing
  - updated
    `lower_prepared_entry_alloca_function_inputs(...)` and
    `lower_prepared_stack_layout_input(...)` in
    `src/backend/stack_layout/slot_assignment.cpp` to rehydrate production
    `StackLayoutInput` instruction and terminator uses from the already-stored
    `liveness_input` or lowered `backend_cfg_liveness`
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the prepared fallback classification no longer stores
    duplicated use lists while the lowered production stack-layout contract
    still rebuilds instruction and terminator uses from liveness
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded prepared-analysis carrier split slice by
  isolating cached fallback stack-layout block/point state from the public
  `StackLayoutInput` surface while preserving the minimal prepared analysis
  facts currently consumed by overwrite/coalescing:
  - added `PreparedEntryAllocaStackLayoutPoint` and
    `PreparedEntryAllocaStackLayoutBlock` in
    `src/backend/stack_layout/slot_assignment.*` so the prepared fallback
    carrier no longer stores public `StackLayoutPoint`/`StackLayoutBlockInput`
    objects directly
  - updated prepared stack-layout classification lowering and rehydration in
    `src/backend/stack_layout/slot_assignment.cpp` so the cached fallback
    carrier still round-trips the current used-name, pointer-access,
    escape-name, derived-pointer-root, and terminator-use subset back into the
    production `StackLayoutInput`
  - extended
    `tests/backend/backend_shared_util_tests.cpp` with focused coverage proving
    the prepared fallback carrier now owns the dedicated prepared-analysis
    block/point types and still preserves the block payload needed to rehydrate
    the production rewrite-input contract
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded prepared stack-layout carrier narrowing
  slice by splitting prepared entry-alloca classification state away from the
  broader public `StackLayoutInput` contract:
  - added
    `PreparedEntryAllocaStackLayoutClassificationInput` in
    `src/backend/stack_layout/slot_assignment.*` so prepared
    entry-alloca state now stores only `entry_allocas`, block classification
    state, and phi incoming uses instead of caching the full
    `StackLayoutInput` surface
  - updated
    `prepare_module_function_entry_alloca_preparation(...)` and
    `lower_prepared_entry_alloca_function_inputs(...)` to round-trip through
    that reduced classification carrier while continuing to rehydrate the
    production rewrite-input metadata contract from
    `PreparedEntryAllocaStackLayoutMetadata`
  - updated
    `tests/backend/backend_shared_util_tests.cpp` to pin the new prepared
    carrier contract and verify lowerable/fallback preparation still rebuilds
    the production stack-layout inputs correctly
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded prepared-fallback liveness-carrier
  narrowing slice by removing full `BackendCfgFunction` ownership from the
  stored rewrite-preparation state:
  - added `BackendCfgLivenessFunction` in `src/backend/liveness.*` plus a
    shared `lower_backend_cfg_to_liveness_function(...)` seam so backend CFG
    lowering can persist just the entry/block/phi data that
    `lower_backend_cfg_to_liveness_input(...)` actually consumes
  - updated `src/backend/stack_layout/slot_assignment.*` so
    `PreparedEntryAllocaFunctionInputs` now stores optional
    `BackendCfgLivenessFunction` instead of a full `BackendCfgFunction`, while
    fallback preparation still uses the full local backend CFG only where
    stack-layout classification needs pointer/escape metadata
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving fallback preparation now retains only liveness-relevant
    backend-CFG state and still rebuilds the production `LivenessInput`
    contract from that reduced carrier
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded prepared-fallback metadata split slice by
  separating non-classification stack-layout metadata from the backend-CFG-
  owned entry-alloca prep carrier:
  - added `PreparedEntryAllocaStackLayoutMetadata` in
    `src/backend/stack_layout/slot_assignment.*` and taught
    `prepare_module_function_entry_alloca_preparation(...)` to store signature,
    return-type, variadic, and call-result metadata outside the fallback
    `stack_layout_input` while keeping the production per-function BIR path
    unchanged
  - updated `lower_prepared_entry_alloca_function_inputs(...)` so the final
    production rewrite-input contract still rehydrates the full
    `StackLayoutInput` before regalloc/stack-layout planning
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the prepared fallback carrier now keeps signature and
    call-result metadata separate from the backend-CFG-backed classification
    input while still lowering back into the existing rewrite-input contract
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded backend-CFG metadata follow-through slice
  by teaching the prepared fallback carrier to preserve entry-alloca
  pointer/escape classification facts through the backend-owned seam:
  - extended `BackendCfgPoint` in `src/backend/liveness.*` with
    pointer-access, escaped-name, and derived-pointer-root metadata and taught
    `lower_lir_to_backend_cfg(...)` to populate those facts from raw LIR while
    leaving the BIR-owned lowering route unchanged
  - updated `src/backend/stack_layout/analysis.cpp` so
    `lower_function_entry_alloca_stack_layout_input(...)` now rebuilds
    `StackLayoutPoint` pointer-access, escape, and GEP-root metadata from the
    prepared backend CFG instead of dropping those facts on the fallback path
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the backend-CFG-backed fallback path now preserves scalar
    overwrite-before-read pruning, aggregate single-block coalescing, and
    escaped-alloca exclusion without falling back to whole-function raw-LIR
    stack-layout lowering
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded backend-owned stack-layout preparation
  slice by moving prepared fallback block/phi ownership off the raw-LIR
  stack-layout lowering path while keeping entry-alloca metadata intact:
  - added
    `lower_function_entry_alloca_stack_layout_input(const LirFunction&, const BackendCfgFunction&)`
    in `src/backend/stack_layout/analysis.*` so stack-layout preparation can
    now rebuild signature, entry-alloca, block, and phi-use metadata from a
    prepared backend CFG seam instead of lowering whole-function block usage
    directly from raw LIR
  - added `EntryAllocaRewriteStackLayoutSource` in
    `src/backend/stack_layout/slot_assignment.*` and retargeted
    `prepare_module_function_entry_alloca_preparation(...)` so fallback
    entry-alloca rewrite prep now tracks that its stack-layout carrier comes
    from `entry allocas + backend CFG`, while the per-function BIR path keeps
    the existing raw-LIR compatibility source
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the mixed-module fallback path now records the new
    backend-owned stack-layout source and carries block usage through the
    prepared backend CFG seam instead of the raw-LIR stack-layout lowering
    path
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded prepared entry-alloca carrier slice by
  formalizing the mixed-module fallback as one backend-owned preparation seam
  before rewrite-input lowering:
  - added `PreparedEntryAllocaFunctionInputs` plus
    `prepare_module_function_entry_alloca_preparation(...)` and
    `lower_prepared_entry_alloca_function_inputs(...)` in
    `src/backend/stack_layout/slot_assignment.*` so entry-alloca rewrite prep
    now distinguishes between the per-function BIR liveness path and the
    fallback backend-CFG preparation carrier without constructing fallback
    liveness inline
  - kept `prepare_module_function_entry_alloca_inputs(...)` as the production
    wrapper over that prepared carrier so existing rewrite callers still
    consume the same `EntryAllocaRewriteInputs` contract
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving lowerable functions stay on the per-function BIR path,
    fallback functions now retain `StackLayoutInput` plus an explicit
    `BackendCfgFunction` carrier, and the prepared fallback still lowers into
    the existing rewrite-input contract
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded entry-alloca preparation-carrier slice by
  moving production stack-layout rewrite setup behind one shared per-function
  helper while keeping explicit seam tracking for the remaining fallback:
  - added `EntryAllocaRewriteInputs` plus
    `prepare_module_function_entry_alloca_inputs(...)` in
    `src/backend/stack_layout/slot_assignment.*` so the production
    entry-alloca rewrite path now receives one carrier with both
    `StackLayoutInput` and `LivenessInput`, along with an enum recording
    whether the function used the narrower per-function BIR seam or the
    raw-LIR backend-CFG compatibility seam
  - updated `rewrite_module_entry_allocas(...)` to route each function through
    the new helper instead of reconstructing raw-LIR liveness and stack-layout
    inputs inline
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the new carrier chooses per-function BIR for a lowerable
    function, preserves entry-alloca metadata on the blocking fallback path,
    and keeps the existing per-function BIR probe pinned to the actual
    mixed-module blocking function
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded stack-layout liveness audit slice by
  proving the direct-BIR production handoff is not ready for entry-alloca
  preparation yet while still pinning a narrower per-function BIR seam:
  - added
    `stack_layout::try_lower_module_function_to_bir_liveness_input(...)` in
    `src/backend/stack_layout/slot_assignment.*` so the backend now has a
    focused audit helper that lowers one allocalless function plus its
    declaration/context through `try_lower_to_bir(...)` and into the shared
    backend-CFG liveness path
  - extended `tests/backend/backend_shared_util_tests.cpp` with a mixed-module
    regression that proves whole-module BIR lowering can stay blocked by an
    entry-alloca-bearing function while the narrower per-function helper still
    succeeds for a lowerable allocalless function and rejects the blocking
    function
  - audited a production caller conversion in
    `rewrite_module_entry_allocas(...)`, confirmed it regressed stack-layout
    preparation because current BIR liveness does not preserve hoisted
    entry-alloca ownership, and left the production path on the existing
    `LirFunction -> backend CFG -> LivenessInput` seam until a backend-owned
    stack-layout carrier exists
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with the repo’s allowed non-decreasing rule and no new
    failures (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)

- Completed the next Step 6 bounded backend-CFG test-surface retargeting slice
  by moving shared backend liveness/regalloc/stack-layout coverage off the
  raw-LIR helper and onto the backend-owned seam:
  - added a focused `lower_lir_to_backend_cfg(...) ->
    lower_backend_cfg_to_liveness_input(...)` test helper in
    `tests/backend/backend_shared_util_tests.cpp` so the main shared-backend
    liveness/regalloc/stack-layout regressions now exercise the backend-owned
    CFG handoff directly
  - rewired the existing shared liveness, regalloc, merged-clobber, and
    stack-layout prep tests to use that backend-CFG helper instead of calling
    `lower_lir_to_liveness_input(...)` as their primary setup path
  - narrowed raw-LIR helper coverage to one compatibility-equivalence
    regression that proves `lower_lir_to_liveness_input(...)` still matches the
    backend-CFG seam on the phi-join slice while Step 6 keeps it as a shim
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)

- Completed the next Step 6 bounded backend-CFG liveness entry slice by
  introducing a backend-owned lowering seam ahead of interval computation while
  keeping raw LIR only as a compatibility shim:
  - added `BackendCfgFunction` plus `lower_bir_to_backend_cfg(...)`,
    `lower_lir_to_backend_cfg(...)`, and
    `lower_backend_cfg_to_liveness_input(...)` in `src/backend/liveness.*` so
    liveness now has a formal backend-owned CFG input path instead of exposing
    only `LirFunction -> LivenessInput`
  - kept `lower_lir_to_liveness_input(...)` as a thin compatibility helper that
    lowers through the new backend-CFG seam rather than constructing
    `LivenessInput` directly from raw LIR
  - updated `src/backend/stack_layout/slot_assignment.cpp` so the production
    entry-alloca rewrite path now builds backend CFG first and then lowers that
    CFG into `LivenessInput`, removing its direct dependency on the raw-LIR
    liveness entry helper
  - rebuilt the tree and passed `cmake --build build -j8`; targeted/runtime
    tests were intentionally deferred for this slice

- Completed the next Step 6 bounded native-emitter assembly-handoff cleanup
  slice by moving compatibility-only x86/aarch64 `assemble_module(...)`
  routing behind one shared backend seam:
  - added `assemble_target_lir_module(...)` plus shared public-target
    resolution in `src/backend/backend.*` so shared backend code now owns the
    `LirModule -> emitted asm text -> target assembler request/result` handoff
    for the public x86/aarch64 assembly wrappers
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so the public
    `assemble_module(const LirModule&, output_path)` wrappers become thin
    delegates over the shared backend helper instead of pairing local
    `emit_module(...)` calls with target-local assembler dispatch
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the shared assembly helper matches the public x86/aarch64
    wrapper staging contracts and still emits objects for the bounded return-add
    slice
  - rebuilt `backend_shared_util_tests` and passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`

- Completed the next Step 6 bounded public direct-BIR wrapper cleanup slice by
  moving compatibility-only x86/aarch64 direct-BIR route ownership into shared
  backend code:
  - added `emit_target_bir_module(...)` in `src/backend/backend.*` so shared
    backend code now owns the public direct-BIR try-or-throw route contract
    for x86, i686, aarch64, and riscv64 text rendering
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so the public
    `emit_module(const bir::Module&)` wrappers become thin delegates over the
    shared backend helper instead of carrying their own compatibility-only
    direct-BIR route policy
  - extended `tests/backend/backend_bir_pipeline_tests.cpp` with focused
    coverage proving the public x86/aarch64 direct-BIR wrappers now exactly
    match the shared backend route for a supported affine-return module and
    preserve the existing unsupported multi-function rejection contracts
  - rebuilt `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)

- Completed the next Step 6 bounded public target-emitter entry-wrapper
  cleanup slice by moving compatibility-only x86/aarch64 direct-LIR
  target-selection/route policy into shared backend code:
  - added `emit_target_lir_module(...)` in `src/backend/backend.*` so shared
    backend code now owns the public target-emitter `LirModule` entry route,
    including the x86 family’s `i686` vs `x86_64` target selection from the
    module triple
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so the public
    `emit_module(const LirModule&)` wrappers become thin delegates over the
    shared backend helper instead of carrying their own backend-entry policy
  - extended `tests/backend/backend_bir_pipeline_tests.cpp` with focused
    coverage proving the x86 public LIR entry now exactly matches the shared
    backend `i686` routing path for a lowerable two-argument add module
  - rebuilt `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)

- Completed the next Step 6/7 bounded prepared direct-LIR rejection ownership
  cleanup slice by deleting the compatibility-only target-local throwing
  adapters once shared backend routing already owned support probing:
  - updated `src/backend/backend.cpp` so the shared backend entrypoint now
    throws the final x86/aarch64 unsupported direct-LIR rejection itself after
    explicit prepared direct-LIR support probing returns `nullopt`
  - removed the now-unused x86/aarch64
    `emit_prepared_lir_module(const LirModule&)` declarations and definitions
    from `src/backend/x86/codegen/emit.*` and
    `src/backend/aarch64/codegen/emit.*`, leaving the target emitters with the
    explicit `try_emit_prepared_lir_module(...)` probe as their only prepared
    direct-LIR routing surface
  - extended `tests/backend/backend_bir_pipeline_tests.cpp` to pin the exact
    shared-backend-owned x86/aarch64 unsupported direct-LIR rejection
    contracts
  - rebuilt `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6/7 bounded prepared direct-LIR support-probe cleanup
  slice by exposing explicit native fallback support probes before the shared
  backend asks target emitters to throw:
  - added explicit x86/aarch64
    `try_emit_prepared_lir_module(const LirModule&)` probes in
    `src/backend/x86/codegen/emit.*` and
    `src/backend/aarch64/codegen/emit.*`, leaving the existing throwing
    `emit_prepared_lir_module(const LirModule&)` wrappers as thin adapters over
    the new optional render surface
  - updated `src/backend/backend.cpp` so prepared direct-LIR fallback routing
    now consults the shared optional probe first and only re-enters the
    throwing target-local wrappers for the final unsupported direct-LIR error
  - extended `tests/backend/backend_bir_pipeline_tests.cpp` with focused x86
    and aarch64 regressions proving the prepared direct-LIR support probes
    return native renderings for bounded fallback modules and `nullopt` for
    unsupported floating-return fixtures
  - rebuilt `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6/7 bounded direct-BIR support-probe cleanup slice by
  replacing the shared backend's exception-text retry seam with explicit native
  BIR support probing before prepared direct-LIR fallback:
  - added explicit x86/aarch64
    `try_emit_module(const bir::Module&)` probes in
    `src/backend/x86/codegen/emit.*` and
    `src/backend/aarch64/codegen/emit.*`, leaving the existing throwing
    `emit_module(const bir::Module&)` wrappers as thin adapters over the new
    optional render surface
  - updated `src/backend/backend.cpp` so direct BIR input still throws through
    the target-local emitters, while LIR-owned routing now uses the shared
    optional BIR probe directly and falls back to prepared direct LIR without
    parsing target-local rejection text
  - extended `tests/backend/backend_bir_pipeline_tests.cpp` with focused x86
    and aarch64 regressions proving the new support probes return native
    renderings for a supported affine-return BIR module and `nullopt` for the
    unsupported multi-function BIR fixture
  - rebuilt `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6/7 bounded backend-entry routing ownership slice by
  moving target `LirModule -> prepare -> try_lower_to_bir -> direct native
  fallback` control into shared backend code and leaving x86/aarch64 emitters
  with prepared direct-LIR fallback ownership only:
  - added `emit_prepared_lir_module(...)` in
    `src/backend/x86/codegen/emit.*` and
    `src/backend/aarch64/codegen/emit.*` so target emitters expose only the
    prepared direct-LIR/native fallback surface needed by shared backend
    routing
  - updated `src/backend/backend.cpp` so the shared backend entrypoint now owns
    direct-BIR subset retry handling for LIR input and dispatches prepared
    direct-LIR fallback through the new target-local prepared helper instead of
    re-entering the public target emitter wrappers
  - updated `src/codegen/llvm/llvm_codegen.cpp` so production `--codegen asm`
    routing now goes through the shared backend LIR entrypoint, while keeping
    the existing riscv64 early rejection for unsupported asm cases that never
    lower to BIR
  - extended
    `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` and
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` with focused
    regressions proving the public target emitter LIR wrappers now match the
    shared backend entrypoint exactly for a lowerable direct-call module
  - rebuilt `backend_bir_tests`, `backend_shared_util_tests`, and `c4cll`;
    passed the focused x86/aarch64 backend suites plus the targeted riscv64
    route/unsupported-asm coverage; rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`, same 32 known failing tests as
    `test_fail_before.log`)
- Completed the next Step 6/7 bounded shared-backend preparation routing slice
  by making target-specific LIR preparation the default shared backend entry
  step before BIR lowering:
  - updated `src/backend/backend.cpp` so
    `backend::emit_module(...)` now runs
    `prepare_lir_module_for_target(...)` before attempting shared BIR lowering,
    carries the prepared module through the direct native fallback path, and
    keeps the riscv64 LLVM-text fallback keyed off the prepared module state
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the shared target-preparation seam turns the dead-local
    entry-alloca fixture from non-lowerable raw LIR into a BIR-lowerable module
    for both x86 and aarch64 targets
  - rebuilt `backend_shared_util_tests` and `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$|^backend_bir_tests$' --output-on-failure`,
    refreshed `test_fail_after.log`, and passed the regression guard with no
    new failures and no pass-count drop (`2809 -> 2809`, same 32 known failing
    tests as `test_fail_before.log`)
- Completed the next Step 6/7 bounded native-emitter preparation ownership
  cleanup slice by moving the duplicated target-local entry-alloca rewrite prep
  behind one shared backend helper:
  - added `prepare_lir_module_for_target(...)` in `src/backend/backend.*` so
    target-specific regalloc/callee-saved configuration for temporary
    entry-alloca rewrites now lives in one shared backend-owned seam
  - updated `src/backend/x86/codegen/emit.cpp` and
    `src/backend/aarch64/codegen/emit.cpp` so both native emitters call the
    shared helper instead of open-coding the same rewrite-module preparation
    flow locally
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving shared backend prep preserves declarations, prunes dead
    param allocas for x86/aarch64 native emission, and leaves riscv64 text
    fallback modules untouched
- Completed the next Step 6/7 bounded direct-emitter module rewrite cleanup
  slice by deleting the duplicated target-local `prune_module_entry_allocas(...)`
  / `prune_dead_entry_allocas(...)` compatibility wrappers and centralizing that
  temporary module-walk seam in shared stack-layout code:
  - added `rewrite_module_entry_allocas(...)` in
    `src/backend/stack_layout/slot_assignment.*` so one shared helper now
    lowers liveness and stack-layout inputs per function, prepares the explicit
    entry-alloca rewrite patch, and applies the isolated raw-LIR mutation
  - updated `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp` so both direct emitters drop their
    duplicated module/function pruning glue and call the shared module rewrite
    helper instead
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused module
    coverage proving the shared helper preserves declarations and prunes dead
    param allocas across the rewritten LIR module
  - rebuilt `backend_shared_util_tests` and `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$|^backend_bir_tests$' --output-on-failure`,
    refreshed `test_fail_after.log`, and passed the regression guard with no
    new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded raw-LIR entry-alloca pruning helper
  retirement slice by deleting the final shared test-only pruning entrypoint
  and forcing that regression coverage onto the backend-owned rewrite-patch
  surface:
  - removed `prune_dead_entry_alloca_insts(...)` plus its private
    `following_entry_store(...)` helper from
    `src/backend/stack_layout/slot_assignment.*`, leaving the explicit
    rewrite-patch builder/apply flow as the only shared entry-alloca pruning
    surface
  - retargeted
    `tests/backend/backend_shared_util_tests.cpp` so the focused dead/live
    entry-alloca pruning regressions now assert through
    `build_entry_alloca_rewrite_patch(...)` and
    `apply_entry_alloca_rewrite_patch(...)` instead of the deleted raw-LIR
    pruning helper
  - rebuilt `backend_shared_util_tests` and `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
    and `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded raw-LIR pruning compatibility cleanup
  slice by deleting the remaining test-only raw-LIR param-pruning helper and
  forcing shared rewrite-patch coverage onto the backend-owned stack-layout
  surface:
  - removed the raw-LIR
    `build_entry_alloca_rewrite_patch(const LirFunction&, ...)` overload and
    `prune_dead_param_alloca_insts(...)` from
    `src/backend/stack_layout/slot_assignment.*`, leaving
    `StackLayoutInput` as the only shared patch-builder input surface
  - updated `tests/backend/backend_shared_util_tests.cpp` so the param-alloca
    pruning coverage and coalesced-entry rewrite coverage now build rewrite
    patches from explicit backend-owned `StackLayoutInput` instead of
    depending on raw-LIR convenience wrappers
  - rebuilt `backend_shared_util_tests` and `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$|^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6/7 bounded entry-alloca patch-preparation ownership
  slice by moving production rewrite-patch construction onto backend-owned
  stack-layout metadata and leaving raw-LIR ownership only in the final apply
  step:
  - added a backend-owned
    `build_entry_alloca_rewrite_patch(const StackLayoutInput&, ...)` path in
    `src/backend/stack_layout/slot_assignment.*` and retargeted
    `prepare_entry_alloca_rewrite_patch(...)` to use
    `StackLayoutInput::entry_allocas` instead of rediscovering entry allocas
    from raw `LirFunction`
  - updated
    `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp` so the production emitters no longer
    pass raw `LirFunction` into rewrite-patch preparation
  - refreshed the focused coverage in
    `tests/backend/backend_shared_util_tests.cpp` so the regression now proves
    patch preparation stays correct when callers provide only backend-owned
    liveness and stack-layout inputs
  - rebuilt `backend_shared_util_tests` and `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
    and `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    refreshed `test_fail_after.log`, and passed the regression guard with no
    new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded entry-alloca compatibility-wrapper
  retirement slice by deleting the final shared raw-LIR convenience wrapper
  once the explicit rewrite-patch seam proved sufficient for every remaining
  caller:
  - removed `apply_entry_alloca_slot_plan(...)` from
    `src/backend/stack_layout/slot_assignment.*` so the shared surface now
    exposes only `build_entry_alloca_rewrite_patch(...)`,
    `prepare_entry_alloca_rewrite_patch(...)`, and
    `apply_entry_alloca_rewrite_patch(...)` for entry-alloca rewrites
  - retargeted the remaining focused coverage in
    `tests/backend/backend_shared_util_tests.cpp` to build the explicit patch
    and apply it directly instead of depending on the deleted convenience
    wrapper
  - rebuilt `backend_shared_util_tests` and `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
    and `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6/7 bounded entry-alloca mutation seam split slice by
  turning the shared prepare-plus-mutate helper into an explicit backend-owned
  rewrite patch plus a thinner raw-LIR applier:
  - added `EntryAllocaRewritePatch`,
    `build_entry_alloca_rewrite_patch(...)`,
    `prepare_entry_alloca_rewrite_patch(...)`, and
    `apply_entry_alloca_rewrite_patch(...)` in
    `src/backend/stack_layout/slot_assignment.*` so the prune/rewrite decision
    can be computed first and the final raw-LIR mutation stays isolated to one
    narrow apply entrypoint
  - retargeted
    `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp` to prepare the entry-alloca rewrite
    patch from backend-owned liveness/stack-layout inputs and then apply it as
    a separate step instead of using the old prepare-and-mutate helper
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving backend-owned inputs can prepare the explicit rewrite patch
    and that the isolated apply step still removes the dead entry alloca as
    expected
  - rebuilt `backend_shared_util_tests` and `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
    and `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded direct-emitter entry-alloca seam cleanup
  slice by moving the final target-local prepare-plus-mutate wiring behind one
  shared stack-layout helper while keeping the actual raw-LIR mutation bounded
  to that shared entrypoint:
  - added
    `prepare_and_apply_entry_alloca_slot_plan(...)` in
    `src/backend/stack_layout/slot_assignment.*` so callers can provide
    backend-owned `LivenessInput`, `StackLayoutInput`, regalloc config, and
    callee-saved metadata while the final entry-alloca mutation remains shared
  - retargeted
    `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp` to call that shared helper instead of
    open-coding plan-bundle construction plus direct
    `apply_entry_alloca_slot_plan(...)` mutation in each emitter
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving backend-owned inputs can drive the shared prepare-and-apply
    entrypoint end to end
  - rebuilt `backend_shared_util_tests` and `backend_bir_tests`, passed
    `ctest --test-dir build -R '^backend_shared_util_tests$' --output-on-failure`
    and `ctest --test-dir build -R '^backend_bir_tests$' --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded AArch64 direct-emitter metadata handoff
  slice by retiring the remaining compatibility-only raw-LIR slot-init rescans
  in favor of the already-lowered stack-layout input:
  - updated
    `src/backend/aarch64/codegen/emit.cpp` so the direct emitter now
    initializes alloca pointer slots from
    `StackLayoutInput::entry_allocas` instead of rescanning
    `fn.alloca_insts`
  - retargeted the direct emitter's lowered param-alloca initialization path
    to use `entry_allocas[].paired_store_value` plus
    `signature_params` metadata instead of rebuilding `%lv.param.*` ownership
    from raw-LIR signature order
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving backend-owned stack-layout input preserves param-alloca
    names, types, and paired stores for that direct-emitter slot-init path
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6/7 bounded target-local entrypoint preparation
  cleanup slice by lifting duplicated liveness/regalloc/stack-layout setup
  behind one shared backend-owned helper while keeping the final mutation step
  on raw LIR:
  - added a new
    `build_stack_layout_plan_bundle(...)` overload in
    `src/backend/stack_layout/slot_assignment.*` that accepts prelowered
    `LivenessInput`, `StackLayoutInput`, `RegAllocConfig`, asm clobbers, and
    callee-saved registers, then runs regalloc-plus-stack-layout bundle setup
    from that backend-owned surface
  - retargeted
    `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp`'s target-local
    `prune_dead_entry_allocas(...)` entrypoints to use that shared helper
    instead of open-coding the regalloc/bundle preparation sequence in each
    emitter
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the shared prep overload preserves dead-param
    classification plus entry/param alloca pruning decisions when callers
    provide backend-owned liveness and stack-layout inputs
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded raw-LIR compatibility cleanup slice by
  retiring the remaining shared-test wrappers and target-local regalloc helpers
  that no longer needed `LirFunction` once callers already lowered backend-owned
  inputs:
  - removed the local raw-`LirFunction` stack-layout helper wrappers from
    `tests/backend/backend_shared_util_tests.cpp` and updated the affected
    shared analysis / slot-assignment regressions to lower
    `StackLayoutInput` explicitly before calling the shared helpers
  - removed the dead
    `run_shared_aarch64_regalloc(const LirFunction&)` helper from
    `src/backend/aarch64/codegen/emit.cpp`
  - retargeted
    `src/backend/x86/codegen/emit.cpp`'s
    `run_shared_x86_regalloc(...)` helper to consume only the already-lowered
    `LivenessInput`, dropping the unused raw-`LirFunction` parameter from that
    target-local compatibility seam
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded shared liveness/regalloc compatibility
  cleanup slice by retiring the remaining raw-`LirFunction` wrapper overloads
  now that production code already lowers to backend-owned `LivenessInput`:
  - removed the raw-`LirFunction` overloads for
    `compute_live_intervals(...)`,
    `allocate_registers(...)`, and
    `run_regalloc_and_merge_clobbers(...)` from
    `src/backend/liveness.*`,
    `src/backend/regalloc.*`, and
    `src/backend/generation.*`, leaving the backend-owned
    `LivenessInput` entrypoints as the canonical shared liveness/regalloc
    surface
  - updated `tests/backend/backend_shared_util_tests.cpp` so the shared
    liveness, regalloc, regalloc/clobber-merge, and downstream stack-layout
    regressions lower `LirFunction` fixtures into `LivenessInput` explicitly
    before invoking the shared helpers
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded shared stack-layout compatibility cleanup
  slice by retiring the last raw-`LirFunction`
  `compute_coalescable_allocas(...)` wrapper after confirming it was only
  serving shared-util test compatibility:
  - removed the raw-`LirFunction`
    `compute_coalescable_allocas(...)` overload from
    `src/backend/stack_layout/alloca_coalescing.*`, leaving the
    `StackLayoutInput` entrypoint as the canonical shared alloca-coalescing
    surface
  - updated `tests/backend/backend_shared_util_tests.cpp` so the dead,
    single-block, and escaped alloca coalescing regressions now lower
    functions into explicit `StackLayoutInput` before invoking shared analysis
    and coalescing helpers
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded shared stack-layout compatibility cleanup
  slice by retiring the remaining analysis/planning wrappers over raw
  `LirFunction` now that production emitters already consume lowered
  `StackLayoutInput`:
  - removed the raw-`LirFunction`
    `analyze_stack_layout(...)`,
    `build_stack_layout_plan_bundle(...)`,
    `plan_entry_alloca_slots(...)`, and
    `plan_param_alloca_slots(...)` overloads from
    `src/backend/stack_layout/analysis.*` and
    `src/backend/stack_layout/slot_assignment.*`, leaving the backend-owned
    `StackLayoutInput` entrypoints as the canonical shared analysis/planning
    surface
  - updated `tests/backend/backend_shared_util_tests.cpp` so the shared
    stack-layout utility regressions now lower functions into
    `StackLayoutInput` explicitly before invoking the shared analysis/planning
    helpers, while preserving the LIR-mutating prune/apply validation paths
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`, same 32 known failing tests as `test_fail_before.log`)
- Completed the next Step 6 bounded production regalloc handoff slice by
  retargeting target emitters to consume backend-owned liveness lowering
  instead of the raw-`LirFunction` regalloc wrapper:
  - updated `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp` so their shared pruning/regalloc paths
    lower `LivenessInput` first and then call
    `run_regalloc_and_merge_clobbers(...)` through the backend-owned input
    overload rather than the compatibility wrapper over raw `LirFunction`
  - added focused coverage in `tests/backend/backend_shared_util_tests.cpp`
    proving the regalloc/clobber handoff still preserves the expected
    call-spanning assignment, spill behavior, merged callee-saved view, and
    cached liveness when callers provide prelowered `LivenessInput`
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`)
- Completed the follow-up Step 6/7 target-local stack-slot ABI audit slice by
  removing the remaining direct AArch64 return-type support gate reparse and
  keeping that stack-slot admission check on backend-owned lowered metadata:
  - taught `src/backend/aarch64/codegen/emit.cpp`'s
    `try_emit_general_lir_asm(...)` unsupported-return filter to consume
    `lower_lir_to_stack_layout_input(fn).return_type` instead of reparsing
    `fn.signature_text` for the direct emitter's aggregate/sret return gate
  - extended `tests/backend/backend_shared_util_tests.cpp` with focused
    coverage proving the backend-owned stack-layout lowering preserves the
    aggregate return/signature metadata needed by that AArch64 gate
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`)
- Completed the next Step 6/7 bounded AArch64 stack-slot metadata handoff
  slice by moving signature and aggregate call-result discovery onto
  backend-owned stack-layout input and retargeting the direct emitter to
  consume it:
  - added
    `StackLayoutSignatureParam`, `StackLayoutCallResultInput`, plus
    `StackLayoutInput` fields for parsed signature params, return type,
    variadic state, and lowered call-result metadata in
    `src/backend/stack_layout/analysis.*`
  - taught
    `src/backend/stack_layout/analysis.cpp`'s
    `lower_lir_to_stack_layout_input(...)` to preserve function-signature
    metadata and stack-backed `LirCallOp` result types alongside the existing
    entry-alloca/CFG lowering
  - retargeted
    `src/backend/aarch64/codegen/emit.cpp`'s
    `gen_build_slots(...)`, sret setup, variadic save-area decision, and
    parameter-slot initialization to consume `StackLayoutInput` metadata
    instead of reparsing or rescanning raw `LirFunction`
  - extended `tests/backend/backend_shared_util_tests.cpp` so the shared
    backend-owned input regression now proves signature params, return-type /
    variadic flags, and aggregate call-result metadata survive the lowering
    handoff needed by downstream slot builders
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`)
- Completed the next Step 6/7 target-local stack-slot handoff slice by moving
  the AArch64 direct-emitter slot builder's generic value-name and entry-alloca
  discovery onto backend-owned stack-layout input while keeping the remaining
  ABI-specific signature/call-result reads bounded to the original
  `LirFunction`:
  - added
    `collect_stack_layout_value_names(...)` in
    `src/backend/stack_layout/analysis.*` so lowered `StackLayoutInput` now
    exposes the value-name inventory needed by downstream slot builders without
    rescanning raw LIR instruction variants
  - taught `src/backend/aarch64/codegen/emit.cpp`'s
    `gen_build_slots(...)` to consume lowered stack-layout input for generic SSA
    slot discovery and entry-alloca data-slot planning, leaving only the
    bounded signature/call-result aggregate ABI cases on raw `LirFunction`
  - extended `tests/backend/backend_shared_util_tests.cpp` so the shared
    backend-owned input regression now proves the lowered stack-layout surface
    preserves entry allocas, paired param stores, derived pointer roots, and
    body SSA value names for downstream target slot builders
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`)
- Completed the next Step 6/7 backend-owned stack-layout planning slice by
  introducing one shared planner bundle over `StackLayoutInput` and routing the
  production emitter pruning setup through it while keeping raw-`LirFunction`
  stack-layout entrypoints as compatibility wrappers:
  - added `StackLayoutPlanBundle` plus
    `build_stack_layout_plan_bundle(...)` in
    `src/backend/stack_layout/slot_assignment.*` so one backend-owned helper
    now computes stack-layout analysis together with entry-alloca and param
    alloca pruning plans from the lowered stack-layout surface
  - taught `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp` to use that shared backend-owned
    planning helper before mutating the original LIR with
    `apply_entry_alloca_slot_plan(...)`
  - extended `tests/backend/backend_shared_util_tests.cpp` so the shared bundle
    regression proves the same lowered stack-layout input preserves both dead
    param-alloca planning decisions and the production entry-alloca mutation
    step
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`)
- Completed the next Step 6/7 production stack-layout handoff slice by
  threading the backend-owned stack-layout input through the AArch64/x86
  entry-alloca pruning path while keeping raw-`LirFunction` stack-layout
  helpers as compatibility wrappers:
  - taught `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp` to lower `LirFunction` once into
    `StackLayoutInput`, then drive shared stack-layout analysis and
    entry-slot planning from that backend-owned surface before mutating the
    original LIR with `apply_entry_alloca_slot_plan(...)`
  - extended `tests/backend/backend_shared_util_tests.cpp` so the backend-owned
    stack-layout regression now covers the full analyze+plan+apply entry-alloca
    pruning flow instead of only the planning decision
  - rebuilt `backend_shared_util_tests`, passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`,
    rebuilt the full tree, refreshed `test_fail_after.log`, and passed the
    regression guard with no new failures and no pass-count drop
    (`2809 -> 2809`)
- Completed the next Step 6/7 backend-owned stack-layout handoff slice by
  moving the shared stack-layout analysis/coalescing/slot-assignment planning
  core behind a backend-owned input surface while preserving raw-`LirFunction`
  entrypoints as lowering wrappers:
  - added `StackLayoutInput`, `EntryAllocaInput`, `StackLayoutBlockInput`,
    `StackLayoutPoint`, `PointerAccess`, and `PhiIncomingUse` in
    `src/backend/stack_layout/analysis.hpp` plus
    `lower_lir_to_stack_layout_input(...)` in
    `src/backend/stack_layout/analysis.cpp` so the stack-layout helpers can
    consume backend-owned use/access/root metadata instead of rescanning raw
    `LirFunction`
  - taught `src/backend/stack_layout/analysis.cpp` to compute use blocks, dead
    param allocas, and first-access overwrite detection from that new
    backend-owned surface while keeping
    `analyze_stack_layout(const LirFunction&, ...)` as a compatibility wrapper
  - added matching backend-owned overloads in
    `src/backend/stack_layout/alloca_coalescing.*` and
    `src/backend/stack_layout/slot_assignment.*` so the shared alloca
    coalescing and slot-planning core can already run from the lowered
    stack-layout surface without changing production target call sites yet
  - added focused shared util coverage in
    `tests/backend/backend_shared_util_tests.cpp` proving backend-owned
    stack-layout input preserves phi-edge use attribution, single-block alloca
    reuse classification, scalar overwrite-before-read slot pruning, and dead
    param alloca planning
  - rebuilt `backend_shared_util_tests` and passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- Completed the first Step 6 MIR-owned liveness/regalloc handoff seam by
  moving the shared dataflow/regalloc core behind a backend-owned input
  surface while preserving the existing LIR entrypoints as wrappers:
  - added `LivenessInput`, `LivenessBlockInput`, and `LivenessPoint` in
    `src/backend/liveness.hpp` plus `lower_lir_to_liveness_input(...)` so the
    live-interval walk can consume backend-owned CFG/use-def data instead of
    requiring raw `LirFunction`
  - taught `src/backend/liveness.cpp` to compute intervals from that new
    backend-owned surface and kept `compute_live_intervals(const LirFunction&)`
    as a compatibility shim that lowers LIR into the new input
  - added matching backend-owned overloads in `src/backend/regalloc.*` and
    `src/backend/generation.*` so the shared regalloc path can already run from
    the lowered liveness surface without introducing target-specific changes
  - added focused shared util coverage in
    `tests/backend/backend_shared_util_tests.cpp` proving phi-join interval
    ranges and call-crossing regalloc behavior are preserved when the shared
    surfaces consume `LivenessInput` instead of raw LIR
  - rebuilt `backend_shared_util_tests` and passed
    `ctest --test-dir build -R backend_shared_util_tests --output-on-failure`
- Completed the follow-up Step 5 audit for parameter-fed branch-only
  conditional-return goto chains with deeper mixed arm lengths and confirmed
  the shared BIR matcher already owns that reordered CFG family:
  - added focused shared lowering coverage in
    `tests/backend/backend_bir_lowering_tests.cpp` proving that an
    interleaved mixed-depth conditional-return LIR slice comparing `%lhs`
    against `%rhs` still normalizes to one canonical
    `bir.select ...; bir.ret ...` block even when the true arm walks a
    deeper three-bridge chain and the false arm uses a shorter one-bridge
    chain
  - added focused AArch64 pipeline coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the native
    emitter routes that parameter-fed mixed-depth fixture through the shared
    BIR-owned `.Lselect_*` path, preserves the `%lhs`/`%rhs` compare operands,
    and no longer exposes any of the original `then.bridge*`, `else.bridge0`,
    `then.ret`, or `else.ret` labels
  - rebuilt `backend_bir_tests`, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new
    failures and no pass-count drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 branch-only conditional-return CFG seam
  behind the shared BIR boundary by removing the shared matcher's physical
  block-order dependency for mirrored false-arm goto chains:
  - rewrote
    `src/backend/lowering/lir_to_bir.cpp`'s
    `try_lower_conditional_return_select_function(...)` return-arm walk to
    resolve empty `br` chains by label and verify the consumed arm blocks cover
    the full non-entry CFG, instead of requiring the true and false arms to
    appear in one fixed contiguous order in `function.blocks`
  - added focused lowering coverage in
    `tests/backend/backend_bir_lowering_tests.cpp` proving that a mirrored
    false-arm double-empty-bridge conditional-return LIR slice now normalizes
    to one canonical `bir.select ...; bir.ret ...` block instead of missing
    shared lowering
  - added focused AArch64 pipeline coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the native
    emitter now routes that reordered false-arm goto-chain fixture through the
    shared BIR-owned `.Lselect_*` path rather than exposing the old direct-LIR
    `else.bridge*` and reordered `then.ret` labels
  - rebuilt `backend_bir_tests`, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new
    failures and no pass-count drop (`2809 -> 2809`)
- Completed the follow-up Step 5 audit for interleaved branch-only
  conditional-return goto chains and confirmed the shared BIR matcher already
  owns that reordered CFG family:
  - added focused shared lowering coverage in
    `tests/backend/backend_bir_lowering_tests.cpp` proving that an
    interleaved two-arm double-empty-bridge conditional-return LIR slice still
    normalizes to one canonical `bir.select ...; bir.ret ...` block even when
    the true-arm and false-arm empty chains are physically interleaved in
    `function.blocks`
  - added focused AArch64 pipeline coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the native
    emitter routes that interleaved branch-only fixture through the shared
    BIR-owned `.Lselect_*` path rather than exposing any of the original
    `then.bridge*`, `else.bridge*`, `then.ret`, or `else.ret` labels
  - rebuilt `backend_bir_tests` and confirmed the new interleaved audit slice
    passes without needing further matcher changes
- Closed the next Step 5 AArch64 branch-only goto-chain CFG seam behind the
  shared BIR boundary by teaching shared lowering to consume linear empty
  chains instead of stopping after one bridge block:
  - widened `src/backend/lowering/lir_to_bir.cpp`'s
    `try_lower_conditional_return_select_function(...)` matcher so the shared
    BIR path now walks a linear chain of empty `br` blocks on each conditional
    arm before the final `ret`, rather than accepting only a direct return or
    a single empty bridge
  - added focused lowering coverage in
    `tests/backend/backend_bir_lowering_tests.cpp` proving that a
    double-empty-bridge conditional-return LIR slice now normalizes to one
    canonical `bir.select ...; bir.ret ...` block instead of missing shared
    lowering
  - added focused AArch64 pipeline coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the native
    direct emitter now routes that longer empty goto-chain fixture through the
    shared BIR-owned `.Lselect_*` path rather than exposing the old direct-LIR
    bridge and return labels
  - rebuilt `backend_bir_tests`, re-ran it, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new
    failures and no pass-count drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 branch-only conditional-return CFG seam
  behind the shared BIR boundary by teaching shared lowering to accept
  asymmetric empty-bridge arms:
  - widened `src/backend/lowering/lir_to_bir.cpp`'s
    `try_lower_conditional_return_select_function(...)` matcher so the shared
    BIR path now accepts the four-block conditional-return family where one
    `condbr` arm returns directly and the other passes through an empty bridge
    block before the final return
  - added focused shared lowering coverage in
    `tests/backend/backend_bir_lowering_tests.cpp` proving that asymmetric
    empty-bridge conditional-return LIR now normalizes to one canonical
    `bir.select ...; bir.ret ...` block instead of missing shared lowering
  - added focused AArch64 pipeline coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the native
    direct emitter now routes that asymmetric conditional-return fixture
    through the shared BIR-owned `.Lselect_*` path rather than exposing the
    old direct-LIR bridge and return labels
  - rebuilt `backend_bir_tests`, re-ran it, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new
    failures and no pass-count drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 branch-only conditional-return CFG seam behind
  the shared BIR boundary by teaching shared lowering to collapse an empty
  bridge-only ternary into the canonical select form:
  - widened `src/backend/lowering/lir_to_bir.cpp`'s
    `try_lower_conditional_return_select_function(...)` matcher so the shared
    BIR path now accepts the minimal five-block conditional-return family where
    each `condbr` arm passes through an empty bridge block before the final
    return block
  - added focused shared lowering coverage in
    `tests/backend/backend_bir_lowering_tests.cpp` proving that empty-bridge
    conditional-return LIR now normalizes to one canonical
    `bir.select ...; bir.ret ...` block instead of staying stranded on a
    target-local CFG surface
  - added focused AArch64 pipeline coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the native
    direct emitter now routes that empty-bridge conditional-return fixture
    through the shared BIR-owned `.Lselect_*` path rather than exposing the
    old bridge labels
  - rebuilt `backend_bir_tests`, re-ran it, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 generic-emitter constant-selector `switch`
  seam behind the shared BIR boundary by widening the existing fence beyond the
  hard-coded two-case layout:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving a
    single-case constant-selector direct-LIR `switch` module still misses
    `try_lower_to_bir(...)` and must now fail explicitly instead of lowering
    through the generic AArch64 direct emitter
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject the broader
    synthetic constant-selector switch-return family when the selector is an
    immediate `i32` and every target/default block is an empty return block,
    keeping that CFG ownership behind the shared BIR boundary without reviving
    target-local switch lowering after a shared BIR miss
  - rebuilt `backend_bir_tests`, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 synthetic switch seam behind the shared BIR
  boundary by widening the existing alloca-backed fence beyond the original
  two-case fixture:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving a
    single-case alloca-backed direct-LIR `switch` module still misses
    `try_lower_to_bir(...)` and must now fail explicitly instead of lowering
    through the generic AArch64 direct emitter
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject the broader
    synthetic alloca-backed switch-return family when every switch target is an
    empty return block, keeping that CFG ownership behind the shared BIR
    boundary without touching broader production switch handling
  - rebuilt `backend_bir_tests`, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Fenced the next narrow Step 5 AArch64 generic-emitter switch seam behind the
  shared BIR boundary:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving a
    constant-selector direct-LIR `switch` module still misses
    `try_lower_to_bir(...)` and must now fail explicitly instead of lowering
    through the generic AArch64 direct emitter
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject only that narrow
    synthetic constant-selector switch-return fallback before
    `try_emit_general_lir_asm(...)` so Step 5 closes another audited switch CFG
    seam without broadening the prune to unrelated direct-LIR coverage
  - rebuilt `backend_bir_tests`, rebuilt the full tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 multi-block switch seam behind the shared BIR
  boundary:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving an
    alloca-backed `LirSwitch` module still misses `try_lower_to_bir(...)` and
    must now fail explicitly instead of reviving target-local switch lowering
    through `try_emit_general_lir_asm(...)`
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject only that narrow
    synthetic alloca-backed switch-return fallback before
    `try_emit_general_lir_asm(...)` so Step 5 closes the bounded CFG seam
    without regressing the broader production switch surface that still lowers
    natively today
  - re-ran `backend_bir_tests`, rebuilt the tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Pruned the redundant AArch64 direct-LIR minimal conditional-return helper now
  that the shared BIR route already owns that bounded branch-return CFG shape:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the direct
    emitter's minimal conditional-return LIR entrypoint lowers through
    `try_lower_to_bir(...)`, collapses to a shared one-block `bir.select`, and
    emits the shared BIR-owned select labels instead of the original direct-LIR
    branch block labels
  - removed the now-redundant
    `parse_minimal_conditional_return_slice(...)` fallback from
    `src/backend/aarch64/codegen/emit.cpp`'s direct-LIR helper path so Step 5
    no longer keeps a duplicate backend-owned branch-return island after the
    outer BIR retry
  - rebuilt `backend_bir_tests`, re-ran it, refreshed `test_fail_after.log`,
    and passed the regression guard with no new failures and no pass-count
    drop (`2809 -> 2809`)
- Finished the follow-up AArch64 helper re-audit after that prune:
  - deleted the now-unused direct-LIR
    `parse_minimal_conditional_return_slice(const LirModule&)` helper from
    `src/backend/aarch64/codegen/emit.cpp`; the direct emitter no longer calls
    it after the shared BIR retry took ownership of that bounded branch-return
    shape
  - deleted the adjacent unused `MinimalConditionalPhiJoinSlice` scaffolding in
    the same file, which had become dead once the direct emitter started
    rejecting target-local phi revival after BIR misses
  - next audit target remains the generic AArch64 direct emitter's broader
    multi-block CFG surface rather than any remaining dedicated helper parser
  - rebuilt the tree, re-ran `backend_bir_tests`, refreshed
    `test_fail_after.log`, and kept the full-suite baseline flat at 32 failing
    tests out of 2841 with no new failures
- Closed the branch-only constant-return Step 5 CFG seam behind shared BIR:
  - taught `src/backend/lowering/lir_to_bir.cpp` to collapse the existing
    `goto`-only empty-block constant-return LIR shape into the canonical
    single-block immediate-return BIR form instead of leaving it stranded on
    target-local direct-LIR handling
  - added focused lowering coverage in
    `tests/backend/backend_bir_lowering_tests.cpp` proving
    `try_lower_to_bir(...)` now accepts that branch-only constant-return slice
    and normalizes it to one shared `bir.ret i32 9` block
  - updated
    `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` and
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` so the native
    emitters now must route the old `goto`-only constant-return fixture through
    the shared BIR-owned immediate-return path instead of target-local
    direct-LIR CFG behavior
  - re-ran `backend_bir_tests`, rebuilt the tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Closed the matching AArch64 Step 5 direct-LIR parity gap for the remaining
  double-indirect local-pointer conditional-return CFG fallback:
  - added focused AArch64 coverage in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` proving the
    double-indirect local-pointer conditional-return fixture still misses
    `try_lower_to_bir(...)` and must now fail explicitly instead of reviving
    target-local multi-block CFG handling
  - taught `src/backend/aarch64/codegen/emit.cpp` to reject that narrow
    fallback shape before `try_emit_general_lir_asm(...)` can emit it, keeping
    ownership behind the shared BIR boundary after the route misses lowering
  - rebuilt `backend_bir_tests`, re-ran it, refreshed `test_fail_after.log`,
    and passed the regression guard with no new failures and no pass-count
    drop (`2809 -> 2809`)
- Closed the next Step 5 AArch64 target-entrypoint CFG asymmetry without
  regressing the broader direct-LIR surface:
  - restricted `src/backend/aarch64/codegen/emit.cpp`'s
    `try_constant_fold_single_block(...)` helper to actual one-block modules so
    the old branch-following `goto`-only constant-return fallback no longer
    folds multi-block CFG chains after they miss shared BIR lowering
  - added a focused regression in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` that constructs a
    `goto`-only constant-return LIR module and proves the AArch64 direct
    emitter now stays on the explicit branch-emission path instead of
    collapsing the CFG to an immediate-return constant-fold
  - re-ran `backend_bir_tests`, rebuilt the tree, refreshed
    `test_fail_after.log`, and passed the regression guard with no new failures
    and no pass-count drop (`2809 -> 2809`)
- Closed the x86 target-entrypoint preparation gap for Step 5 CFG ownership:
  - added a focused regression in
    `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` proving a minimal
    helper-call module with a dead entry alloca still misses raw
    `try_lower_to_bir(...)` but must route through the native x86 backend after
    backend-side pruning
  - taught `src/backend/x86/codegen/emit.cpp` to prune dead entry allocas
    before both the shared BIR retry and the direct-LIR fallback so x86 no
    longer diverges from the AArch64 entrypoint on this ownership seam
  - re-ran `backend_bir_tests` for the x86 entrypoint-pruning slice before the
    full-suite regression pass
  - regenerated the slice-local full-suite baseline and after-log on the
    rebuilt tree, then passed the regression guard with no new failures and no
    pass-count drop (`2809 -> 2809`); the standing 32 failures remain the same
    pre-existing AArch64 variadic set in both logs
- Audited the remaining Step 5 AArch64 target-local phi seam and chose the
  direct production prune:
  - rejected `LirPhiOp` in
    `src/backend/aarch64/codegen/emit.cpp`'s general direct-LIR emitter so
    modules that miss `try_lower_to_bir(...)` can no longer revive
    predecessor-copy phi lowering past the shared BIR boundary
  - deleted the now-dead AArch64 predecessor-copy collection/emission scaffold
    from the general direct-LIR emitter
  - added a focused regression in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` that constructs an
    alloca-backed conditional-phi join which bypasses `try_lower_to_bir(...)`
    and now must fail the AArch64 direct emitter instead of silently lowering
    through the old target-local phi-copy path
  - re-ran `backend_bir_tests` for the AArch64 prune slice before the full-suite
    regression pass
- Audited the remaining Step 5 target-local phi seams and chose the smaller
  production prune for x86:
  - removed `LirPhiOp` interpretation from
    `src/backend/x86/codegen/emit.cpp`'s
    `parse_minimal_double_indirect_local_pointer_conditional_return_imm(...)`
    helper so the legacy direct-LIR constant-folder can no longer revive
    phi-aware CFG ownership after a module misses shared BIR lowering
  - added a focused regression in
    `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` that constructs an
    alloca-backed conditional-phi join which bypasses `try_lower_to_bir(...)`
    and now must fail the x86 direct emitter instead of silently constant
    folding through the legacy phi path
  - re-ran `backend_bir_tests` for the x86 prune slice before the full-suite
    regression pass
- Replaced the mislabeled native Step 5 conditional-phi route coverage with a
  real lowerable LIR phi-join fixture in
  `tests/backend/backend_bir_test_support.cpp` and
  `tests/backend/backend_bir_test_support.hpp` so the pipeline tests no longer
  exercise only the already-lowered direct-BIR path
- Switched the x86 and AArch64 native conditional-phi pipeline tests in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` and
  `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` to use that LIR
  fixture and tightened the route assertions so they require the shared BIR
  select labels while rejecting the old target-local predecessor labels
- Re-ran `backend_bir_tests`, refreshed `test_fail_after.log`, and passed the
  regression guard with no new failures and no pass-count drop (`2841 ->
  2841`)
- Closed the remaining Step 5 x86 route-coverage gap in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` by proving the direct
  `c4c::backend::x86::emit_module(...)` LIR entrypoint itself routes the
  lowerable conditional-phi join and `u8` post-join-add slices through the
  shared BIR-owned select/join path instead of only covering the generic
  `c4c::backend::emit_module(...)` front door
- Re-ran `backend_bir_tests`, refreshed `test_fail_after.log`, and passed the
  regression guard with no new failures and no pass-count drop (`2841 ->
  2841`)
- Activated `plan.md` from
  `ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md`
- Reset active execution tracking for the new plan
- Audited the current typed-type boundary and backend ownership seams:
  - `src/codegen/lir/types.hpp` still stores only text plus coarse
    `LirTypeKind`
  - `src/codegen/lir/verify.cpp` only validates emptiness and `void` exclusion
    for `LirTypeRef`
  - `src/backend/lowering/lir_to_bir.cpp` still uses
    `lower_scalar_type_text(std::string_view)` at the main scalar cast/binop/cmp
    /select/return lowering sites
  - `src/backend/lowering/lir_to_bir.cpp` still parses function-signature
    parameter text in the fallback path
  - LIR-owned backend analysis remains live in `src/backend/liveness.cpp`,
    `src/backend/regalloc.cpp`, `src/backend/generation.cpp`, and
    `src/backend/stack_layout/*.cpp`
  - target-local phi cleanup is still present in
    `src/backend/aarch64/codegen/emit.cpp` and
    `src/backend/x86/codegen/emit.cpp`
- Added exact integer-width payload support to `src/codegen/lir/types.hpp`
  through typed `LirTypeRef` integer metadata plus `LirTypeRef::integer(...)`
- Taught `src/codegen/lir/verify.cpp` to reject integer typed/text drift such
  as width/text mismatches
- Switched the first straight-line scalar lowering sites in
  `src/backend/lowering/lir_to_bir.cpp` from string decoding to typed
  `LirTypeRef` inspection for cast/binop/cmp/select/ret lowering
- Added focused backend tests that cover:
  - typed integer verifier mismatch rejection
  - typed `LirTypeRef::integer(8)` lowering through the existing i8 multiply
    BIR slice
- Converted the conditional select compare/cast lowering paths in
  `src/backend/lowering/lir_to_bir.cpp` from string-based integer decoding to
  typed `LirTypeRef` inspection
- Moved the two-parameter unsigned-char select-phi fixture in
  `tests/backend/backend_bir_test_support.cpp` onto explicit typed
  `LirTypeRef::integer(...)` constructors for its cast/cmp/phi/ret type refs
- Added a focused typed `i8` select regression in
  `tests/backend/backend_bir_lowering_tests.cpp` and kept it on the verified
  BIR path with `c4c::codegen::lir::verify_module(...)`
- Captured full-suite before/after logs in `test_fail_before.log` and
  `test_fail_after.log`
- Passed the regression guard check with no new failures and no pass-count drop
  (`2841 -> 2841`)
- Replaced the remaining `lower_function_params(...)` signature-parameter
  scalar fallback from raw `i8`/`i32`/`i64` string matching to
  `LirTypeRef`-based typed lowering
- Moved the nearby one-parameter `widen_unsigned` `i8` zext BIR fixture onto
  explicit typed `LirTypeRef::integer(...)` cast construction
- Added focused typed zext lowering coverage for the one-parameter `i8`
  straight-line slice and kept it on the verified LIR path
- Re-ran `backend_bir_tests`, nearby backend route tests, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `try_lower_minimal_folded_two_arg_direct_call_module(...)` in
  `src/backend/lowering/lir_to_bir.cpp` to accept structured `LirFunction`
  helper return/param metadata when present instead of hard-requiring raw
  `"i32"` helper signature parameter text
- Replaced the folded-helper binop type check in
  `src/backend/lowering/lir_to_bir.cpp` from raw `"i32"` string comparison to
  typed `LirTypeRef` scalar inspection
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the folded
  two-argument direct-call path working when typed helper params disagree with
  stale signature param text
- Re-ran the focused direct-call backend BIR coverage plus the full suite and
  kept the regression guard passing (`2841 -> 2841`)
- Switched `parse_backend_single_identity_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the identity and
  dual-identity direct-call lowering paths working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `parse_backend_single_add_imm_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the add-immediate
  and call-crossing direct-call lowering paths working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests` plus the full suite and kept the regression guard
  passing (`2841 -> 2841`)
- Switched `parse_backend_two_param_add_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the minimal
  two-argument direct-call lowering path working when typed helper params
  disagree with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched `parse_backend_single_param_slot_add_function(...)` in
  `src/backend/lowering/call_decode.hpp` to prefer structured helper return and
  parameter metadata when present instead of hard-requiring `"i32"` signature
  parameter text
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the slot-based
  add-immediate direct-call lowering path working when the typed helper param
  disagrees with stale signature param text
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched the remaining direct-call helper recognizer typed-operation checks
  in `src/backend/lowering/call_decode.hpp` from raw `"i32"` string matching to
  semantic `LirTypeRef` integer-width inspection for loads, stores, allocas,
  and helper add-rewrite operations
- Added focused shared-util regressions in
  `tests/backend/backend_shared_util_tests.cpp` that keep the single-add,
  slot-add, identity, and two-param helper recognizers working when helper
  operations are built from typed `LirTypeRef::integer(32)` metadata while the
  signature text stays stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Switched the remaining direct-call typed-operation checks in
  `src/backend/lowering/lir_to_bir.cpp` from raw `"i32"` string matching to
  semantic `LirTypeRef` integer-width inspection for direct-call local slots,
  caller stores, dual-identity subtraction, and call-crossing add chains
- Tightened the direct-call typed-operation regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` and
  `tests/backend/backend_shared_util_tests.cpp` so the helper/caller
  `LirTypeRef` objects carry semantic `i32` metadata while their stored text is
  intentionally stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite
  with the regression guard still passing (`2841 -> 2841`)
- Completed the active helper-recognizer audit:
  - no remaining minimal direct-call helper recognizers still rely on
    signature-text parameter typing where structured helper metadata already
    exists
  - pointer payload support is not required for the next lowering slice
    because the remaining typed-lowering/direct-call seams in scope are still
    integer and signature-shape driven; pointer handling remains confined to
    declared-extern parsing and can stay deferred until the first
    pointer-backed BIR consumer is converted
- Switched the widened `i8` instruction-local cast/cmp/phi checks in
  `src/backend/lowering/lir_to_bir.cpp` from raw `i1`/`i8`/`i32` text matching
  to semantic integer-width inspection, including the nearby generic compare
  materialization and conditional-select zext guards
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the widened `i8`
  add, compare-return, and two-parameter select-phi BIR lowering paths working
  when instruction-local `LirTypeRef` objects carry semantic integer widths
  while their stored text is intentionally stale
- Re-ran `backend_bir_tests` plus the full suite and kept the regression guard
  passing (`2841 -> 2841`)
- Switched the widened `i8` add/compare/select helpers in
  `src/backend/lowering/lir_to_bir.cpp` off raw `LirRet.type_str == "i8"`
  gating and onto semantic `LirFunction.return_type` integer-width inspection
- Extended focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` so widened `i8`
  add-return, compare-return, branch-return select, and phi-select lowering
  still succeed when the typed instruction/function metadata is correct but the
  stored return text is intentionally stale
- Re-ran `backend_bir_tests` and `backend_shared_util_tests` for the widened
  `i8` stale-return-text slice before the full-suite check
- Re-ran the full suite into `test_fail_after.log` and passed the regression
  guard with no new failures and no pass-count drop (`2841 -> 2841`)
- Switched the generic single-block fallback in
  `src/backend/lowering/lir_to_bir.cpp` to derive the lowered function return
  type from structured `LirFunction.return_type` metadata first, then bounded
  signature text fallback, before trusting raw `LirRet.type_str`
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the generic
  straight-line add, compare-return, branch-return select, and phi-select BIR
  lowering paths working when instruction-local return/type text is stale but
  the enclosing function or typed `LirTypeRef` metadata still carries the
  correct `i32` semantics
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  and passed the regression guard with no new failures and no pass-count drop
  (`2841 -> 2841`)
- Switched the zero-arg direct-call helper/caller recognizers in
  `src/backend/lowering/call_decode.hpp` and
  `src/backend/lowering/lir_to_bir.cpp` off hard `define i32` / `ret i32`
  gating and onto semantic `LirFunction.return_type` metadata first, with the
  existing signature/ret text fallback still available for untyped cases
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the zero-arg direct
  call lowering path working when either the helper or caller carries semantic
  `i32` return metadata while the stored signature and `ret` text are
  intentionally stale
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the remaining minimal direct-call caller-return gates in
  `src/backend/lowering/lir_to_bir.cpp` from raw `define i32` / `ret i32`
  text checks to `lir_function_matches_minimal_no_param_integer_return(...)`
  plus `lower_function_return_type(...)` for the two-argument, add-immediate,
  identity, dual-identity subtraction, and call-crossing slices
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep those direct-call
  slices lowering when the caller `LirFunction.return_type` still carries `i32`
  semantics but the stored caller signature and `ret` render text are
  intentionally stale
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the minimal extern global-array load and char/int pointer-difference
  recognizers in `src/backend/lowering/lir_to_bir.cpp` off raw
  cast/GEP/binop/cmp/load/ret integer text checks and onto semantic
  `LirTypeRef` width inspection plus structured function return metadata, while
  keeping bounded text fallback for untyped cases
- Completed the first Step 5 AArch64 ownership move:
  - audited `src/backend/backend.cpp`, `src/backend/aarch64/codegen/emit.cpp`,
    and `src/backend/x86/codegen/emit.cpp` for where lowerable phi/select CFG
    shapes still bypass the canonical BIR route
  - found that `src/backend/aarch64/codegen/emit.cpp` still attempted
    target-local direct LIR emission before `try_lower_to_bir(...)`, unlike the
    shared backend entrypoint and x86
  - reordered the AArch64 LIR entrypoint to prefer BIR emission when lowering
    and native BIR emission succeed, while preserving the existing direct-LIR
    fallback when the lowered BIR module is outside the native AArch64 BIR
    subset
  - added an AArch64 backend pipeline regression in
    `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` that proves a
    lowerable conditional phi join now routes through the shared BIR-owned
    select path instead of staying on target-local phi predecessor-copy labels
- Re-ran `backend_bir_tests`, the four AArch64 route/c-testsuite regressions
  uncovered by the entrypoint reordering, and the full suite
- Refreshed `test_fail_after.log` and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the extern
  global-array load and both pointer-difference slices lowering when the
  instruction-local cast/GEP/binop/cmp text and `ret` text are intentionally
  stale but the typed metadata still carries the correct `i8`/`i32`/`i64`
  semantics
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the minimal scalar global-load, extern scalar global-load, and
  scalar global store-reload recognizers in
  `src/backend/lowering/lir_to_bir.cpp` off raw global/load/store/ret `"i32"`
  string checks and onto semantic global `TypeSpec`, instruction `LirTypeRef`,
  and function return metadata with the existing text fallback still available
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep those three scalar
  global slices lowering when the global text, load/store `LirTypeRef` render
  text, and `ret` render text are intentionally stale but the typed metadata
  still says `i32`
- Switched the declared direct-call fixed-param vararg comparison path in
  `src/backend/lowering/lir_to_bir.cpp` to recover fixed call-surface types
  from the backend direct-global typed-call parser when the generic LIR typed
  call parse rejects stale vararg suffix text
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the declared
  direct-call vararg slice lowering when the stored fixed-param suffix text is
  stale but the actual typed call argument still carries the correct fixed
  `i32` surface
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Re-ran `backend_bir_tests` and the full suite, refreshed
  `test_fail_after.log`, and passed the regression guard with no new failures
  and no pass-count drop (`2841 -> 2841`)
- Switched the generic conditional branch-select and phi-select fast paths in
  `src/backend/lowering/lir_to_bir.cpp` off raw `ret.type_str` /
  `phi.type_str` text equality and onto typed scalar checks via
  `lower_function_return_type(...)` and semantic `LirTypeRef` inspection
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the generic
  one-parameter branch-select and phi-select lowering paths working when the
  branch/join return text or phi render text is stale but the typed semantics
  still match the enclosing function
- Re-ran `backend_bir_tests` and `backend_shared_util_tests` for the generic
  select stale-text slice before the full-suite check
- Switched the minimal countdown-loop recognizer in
  `src/backend/lowering/lir_to_bir.cpp` off raw instruction-local/load/store
  /ret `"i32"` text checks and onto semantic `LirTypeRef` integer-width
  inspection plus `lower_function_return_type(...)` for the loop exit return
- Switched the minimal declared direct-call lowering path in
  `src/backend/lowering/lir_to_bir.cpp` off raw caller `define i32`, `ret i32`,
  call `i32`, and declared-callee `declare i32` gates when structured
  `LirFunction` / `LirCallOp` metadata is already present
- Taught `src/codegen/lir/hir_to_lir.cpp` to preserve declaration
  `return_type` and `params` metadata on `LirFunction` declarations instead of
  leaving declaration functions text-only
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the zero-arg
  declared direct-call lowering path working when the caller, declared callee,
  and call instruction carry semantic `i32` metadata while the stored
  signature and return text are intentionally stale
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the bounded
  countdown do-while lowering path working when the loop-local alloca,
  load/store/binop/cmp type refs carry semantic `i32` metadata while their
  stored text, including the exit `ret`, is intentionally stale
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite
  and passed the regression guard with no new failures and no pass-count drop
  (`2841 -> 2841`)
- Added typed `LirExternDecl.return_type` metadata in
  `src/codegen/lir/ir.hpp`, taught `src/codegen/lir/hir_to_lir.cpp` to
  populate it from the existing extern declaration map, and switched the
  minimal declared direct-call extern gate in
  `src/backend/lowering/lir_to_bir.cpp` to prefer that structured return
  metadata before falling back to raw `return_type_str`
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the inferred-param
  extern declared direct-call lowering path working when the extern decl text
  and call/return render text are intentionally stale but the typed extern and
  call metadata still carry `i32` semantics
- Re-ran `backend_bir_tests` and `backend_shared_util_tests` for the typed
  extern declared-direct-call slice before the full-suite check
- Re-ran the full suite into `test_fail_after.log` and passed the regression
  guard with no new failures and no pass-count drop (`2841 -> 2841`)
- Switched the remaining minimal-helper return gates in
  `src/backend/lowering/call_decode.hpp` from raw `ret.type_str == "i32"`
  checks to `backend_lir_lower_function_return_type(...)` for the
  add-immediate, slot-add, identity, and two-parameter helper recognizers
- Added focused shared-util regressions in
  `tests/backend/backend_shared_util_tests.cpp` that keep those helper
  recognizers accepting semantic `i32` helpers when the stored helper return
  text is intentionally stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the backend direct-global typed-call fallback in
  `src/backend/lowering/call_decode.cpp` to return the backend-owned parsed
  view and preserve fixed vararg parameter types from the actual typed call
  args instead of rejecting the parse on stale fixed-param suffix text
- Added a focused shared-util regression in
  `tests/backend/backend_shared_util_tests.cpp` that keeps the direct-global
  typed-call operand helper recovering the fixed typed vararg operand when the
  stored fixed-param suffix text is intentionally stale
- Re-ran `backend_shared_util_tests`, `backend_bir_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Removed the remaining zero-fixed-param declared direct-call fallback gate in
  `src/backend/lowering/lir_to_bir.cpp` that still required raw
  `declare i32 @name()` signature text in the vararg/no-typed-param branch
  even when typed callee return metadata and parsed fixed params already proved
  the declaration surface
- Added a focused backend regression in
  `tests/backend/backend_bir_lowering_tests.cpp` that keeps the zero-arg
  declared direct-call vararg slice lowering when the callee carries semantic
  `i32` return metadata while the stored declaration text is intentionally
  stale (`declare i64 @helper_decl(...)`)
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Switched the remaining caller-local-slot `i32` recognizer guards in
  `src/backend/lowering/call_decode.hpp` and
  `src/backend/lowering/lir_to_bir.cpp` from typed-only checks to semantic
  integer-width matching with bounded `i32` text fallback for stale caller
  alloca/store/load render text
- Added focused backend regressions in
  `tests/backend/backend_bir_lowering_tests.cpp` that keep the two-argument
  local-slot direct-call slice and the add-immediate local-slot direct-call
  slice lowering when the caller alloca/store/load text is stale but typed
  metadata still carries `i32`
- Re-ran `backend_bir_tests`, `backend_shared_util_tests`, and the full suite,
  refreshed `test_fail_after.log`, and passed the regression guard with no new
  failures and no pass-count drop (`2841 -> 2841`)
- Completed the remaining Step 1 helper re-audit in
  `src/backend/lowering/lir_to_bir.cpp`:
  - `lower_binary(...)`, `lower_compare_materialization(...)`, and
    `lower_select_materialization(...)` already lower scalar kinds from
    semantic `LirTypeRef` width metadata through
    `lower_scalar_type(const LirTypeRef&)`
  - the remaining raw-text sites in `lir_to_bir.cpp` are no longer generic
    scalar/materialization helpers; they are specific fallback recognizers or
    backend-ownership seams that belong to the next migration stage
  - no additional generic typed-type conversion remains for Step 1, so the plan
    now advances to BIR-owned phi/CFG normalization instead of stretching the
    typed-type audit further
- Added x86 and AArch64 end-to-end LIR route coverage in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` and
  `tests/backend/backend_bir_pipeline_aarch64_tests.cpp` for the bounded `u8`
  predecessor-add phi-with-post-join-add slice by retargeting the existing LIR
  fixture onto each native backend triple and asserting native emission stays
  on the shared BIR-owned `.Lselect_*` / `.Lselect_join` path instead of
  target-local predecessor labels
- Rebuilt `backend_bir_tests`, re-ran it, re-ran the full suite into
  `test_fail_after.log`, and passed the regression guard in
  `--allow-non-decreasing-passed` mode with no new failures and no pass-count
  drop (`2841 -> 2841`)
- Pruned the remaining x86 CFG-sensitive direct-LIR helper stubs in
  `src/backend/x86/codegen/emit.cpp` by removing the `goto-only` branch walker
  and double-indirect local-pointer conditional-return constant-fold fallback
  from the production direct-emitter entrypoint once those modules miss shared
  BIR lowering
- Added focused regressions in
  `tests/backend/backend_bir_pipeline_x86_64_tests.cpp` that prove both shapes
  now fail with the normal direct-LIR unsupported error instead of reviving
  target-local CFG ownership past the BIR boundary
- Rebuilt `backend_bir_tests`, re-ran it, refreshed `test_fail_after.log`, and
  passed the regression guard with no new failures and no pass-count drop
  (`2809 -> 2809` passes, same 32 pre-existing AArch64 variadic/backend
  failures)

## Next

- audit `src/backend/backend.cpp`, `src/backend/aarch64/codegen/emit.cpp`, and
  `src/backend/x86/codegen/emit.cpp` to classify which phi/CFG-sensitive cases
  still bypass the canonical BIR route or still require target-local cleanup
- choose the narrowest production slice that can move one of those remaining
  phi/CFG responsibilities behind `try_lower_to_bir(...)` without expanding
  scope into MIR-owned liveness/regalloc work
- keep pointer payload support deferred until a concrete pointer-backed BIR
  lowering consumer appears, then add only the narrow typed payload that
  consumer needs

## Blockers

- none recorded

## Resume Notes

- Repo lifecycle state was `no active plan`; this todo starts from activation.
- The source idea fixes migration order as:
  1. typed LIR type ownership
  2. canonical BIR phi/control-flow surface
  3. MIR-owned liveness
  4. MIR-owned regalloc and stack-layout integration
  5. delete temporary LIR-side backend shims
- Use `todo.md` as the canonical execution state even though the reusable skill
  text mentions `plan_todo.md`.
- Immediate validating targets:
  `test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper_with_stale_return_text`
  `test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper_with_stale_return_text`
  `test_backend_shared_call_decode_accepts_typed_i32_identity_helper_with_stale_return_text`
  `test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper_with_stale_return_text`
  and
  `test_lir_verify_rejects_typed_integer_text_mismatch` and
  `test_bir_lowering_accepts_typed_tiny_return_add_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_tiny_return_ne_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_i8_return_add_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_i8_return_ne_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_single_param_select_branch_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_single_param_select_branch_slice_with_stale_return_text`
  `test_bir_lowering_accepts_typed_single_param_select_phi_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_single_param_select_phi_slice_with_stale_phi_text`
  `test_bir_lowering_accepts_typed_two_param_u8_select_ne_branch_slice_with_stale_text`
  and
  `test_bir_lowering_accepts_typed_two_param_u8_select_ne_phi_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_minimal_scalar_global_load_lir_slice_with_stale_text`
  `test_bir_lowering_accepts_typed_minimal_extern_scalar_global_load_lir_slice_with_stale_text`
  and
  `test_bir_lowering_accepts_typed_minimal_scalar_global_store_reload_lir_slice_with_stale_text`
  in
  `tests/backend/backend_bir_lowering_tests.cpp`.
- Latest validating targets for the direct-call helper seam:
  `test_bir_lowering_accepts_minimal_direct_call_add_imm_lir_module_with_typed_helper_param`
  `test_bir_lowering_accepts_minimal_direct_call_add_imm_slot_lir_module_with_typed_helper_param`
  and
  `test_bir_lowering_accepts_minimal_call_crossing_direct_call_lir_module_with_typed_helper_param`
  plus
  `test_bir_lowering_accepts_minimal_two_arg_direct_call_lir_module_with_typed_helper_params`
  in `tests/backend/backend_bir_lowering_tests.cpp`.
- Latest validating targets for the typed helper-operation seam:
  `test_backend_shared_call_decode_accepts_typed_i32_single_add_imm_helper`
  `test_backend_shared_call_decode_accepts_typed_i32_slot_add_helper`
  `test_backend_shared_call_decode_accepts_typed_i32_identity_helper`
  and
  `test_backend_shared_call_decode_accepts_typed_i32_two_param_add_helper`
  in `tests/backend/backend_shared_util_tests.cpp`.
- Latest validating target for the direct-global vararg typed-call seam:
  `test_backend_shared_call_decode_prefers_typed_vararg_call_args_over_stale_fixed_suffix_text`
  in `tests/backend/backend_shared_util_tests.cpp`.
- Latest validating target for the declared direct-call vararg lowering seam:
  `test_bir_lowering_uses_actual_fixed_vararg_call_types_for_declared_direct_calls`
  in `tests/backend/backend_bir_lowering_tests.cpp`.
- Full-suite before/after comparison for this slice should use
  `test_fail_before.log` and `test_fail_after.log` with the regression-guard
  checker script.
