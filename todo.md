Status: Active
Source Idea Path: ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory current publisher families and helper dependencies

# Current Packet

## Just Finished

Completed Step 1 inventory for `src/backend/prealloc/prealloc.cpp` publisher families and helper dependencies.

Publisher families observed:

- Coordinator/public entry: `BirPreAlloc::run()`, `BirPreAlloc::publish_contract_plans()`, `prepare_*_bir_module_with_options()`, and `BirPreAlloc::note()`.
- Identity/coordinator-adjacent facts: `publish_prepared_bir_label_identity()` and `populate_regalloc_placement_identity()`.
- Frame plan: `populate_frame_plan()`, callee-save classification/save-slot helpers, frame slot ordering, dynamic-stack detection, register placement helpers.
- Dynamic stack: `populate_dynamic_stack_plan()`, `dynamic_alloca_type_text()`, dynamic alloca/stacksave/stackrestore scan, frame-plan fixed-slot flag lookup.
- Storage plans: `populate_storage_plans()`, storage encoding/value-home conversion, regalloc value lookup, register/spill placement helpers.
- Call plans: `populate_call_plans()`, direct/indirect callee resolution, memory-return planning, ABI binding lookup, call clobbers, preserved values, symbol pointer source handling.
- Variadic entry plans: `populate_variadic_entry_plans()` plus AAPCS64 register-save/overflow-area/helper-resource/storage-authority helpers.
- Special carriers: `populate_i128_carriers()`, `populate_f128_carriers()`, `populate_atomic_operations()`, `populate_intrinsic_carriers()`, and `populate_inline_asm_carriers()` with each family's validation/build helpers.
- Runtime helpers: `populate_f128_runtime_helper_facts()` and `populate_i128_runtime_helper_lanes()` with ABI binding, marshaling, boundary policy, and call-ownership helpers.

Selected first extraction family: dynamic stack, likely as `src/backend/prealloc/dynamic_stack.cpp`, because the publisher body is compact and depends on a small helper set compared with frame, storage, call, variadic, carrier, or runtime-helper families.

## Suggested Next

Extract `populate_dynamic_stack_plan()` and its purely local `dynamic_alloca_type_text()` helper into `src/backend/prealloc/dynamic_stack.cpp`; add the narrow declaration/build registration needed for `BirPreAlloc::publish_contract_plans()` to call it without changing publication order.

## Watchouts

- Keep this route behavior-preserving; do not change prepared output, phase order, snapshots, or test expectations.
- Do not split `regalloc.cpp` or `prepared_printer.cpp` under this plan.
- Avoid broad private helper headers that become a second monolith.
- Prefer one fact-publisher family per packet unless dependency edges require grouping.
- Dynamic stack extraction still needs access to shared helpers `is_dynamic_alloca_call()`, `find_preferred_block_label_id()`, and `maybe_named_value_id()`; keep any private helper surface narrow and do not turn it into a general prealloc utility dump.
- `is_dynamic_alloca_call()` is also used by `populate_frame_plan()`, so do not move it privately into dynamic stack unless frame keeps a valid declaration or the helper is intentionally placed in a narrow shared private header.
- `populate_dynamic_stack_plan()` reads frame facts via `find_prepared_frame_plan()`, and `publish_contract_plans()` currently runs frame before dynamic stack and then frame again after variadic; preserve that order.
- Larger families to defer: frame plan shares callee-save and placement helpers; call/runtime helpers share `build_call_clobber_set()`, `build_call_preserved_values()`, `find_call_program_point()`, and register/ABI helpers; carriers depend on storage plans and family-specific validation.

## Proof

Inventory-only packet; no implementation files changed and no build run. Intended first-extraction proof command: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
