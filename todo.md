Status: Active
Source Idea Path: ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reduce `prealloc.cpp` to coordinator ownership

# Current Packet

## Just Finished

Completed Step 5 frame-plan publisher extraction. `populate_frame_plan()` now lives in `src/backend/prealloc/frame_plan.cpp`, with the narrow declaration in `src/backend/prealloc/frame_plan.hpp`. The frame-plan-only saved-register placement, callee-save span lookup, register-placement fallback, frame-slot id, saved-register slot sizing, offset alignment, regalloc lookup, and dynamic-stack detection dependencies moved with it. `BirPreAlloc::publish_contract_plans()` still calls the frame-plan publisher in the same two phase positions around dynamic-stack, call-plan, and variadic-plan publishing. `prealloc.cpp` now only coordinates the prealloc route and contract-plan publisher ordering. The existing recursive prealloc source glob picked up `frame_plan.cpp`; no build registration edit was needed.

## Suggested Next

Review Step 5 completion and decide whether the source idea is ready for lifecycle review or needs a final coordinator cleanup packet.

## Watchouts

- Keep this route behavior-preserving; do not change prepared output, phase order, snapshots, or test expectations.
- Do not split `regalloc.cpp` or `prepared_printer.cpp` under this plan.
- Avoid broad private helper headers that become a second monolith.
- Prefer one fact-publisher family per packet unless dependency edges require grouping.
- Frame-plan build registration relies on the existing recursive prealloc source glob; adding `frame_plan.cpp` caused CMake to reconfigure successfully during the proof.
- `is_dynamic_alloca_call()` still has package linkage via `dynamic_stack.hpp` because frame-plan dynamic-stack detection uses it; keep that header narrow.
- Label-identity build registration also relies on the existing recursive prealloc source glob; adding `label_identity.cpp` caused CMake to reconfigure successfully during the proof.
- `find_preferred_block_label_id()` remains duplicated narrowly in `dynamic_stack.cpp` and `label_identity.cpp`; do not introduce a broad shared helper header unless more extracted publishers need it.
- Regalloc placement identity keeps a narrow local copy of its placement/spill-slot/value-id helpers in `regalloc_placement_identity.cpp`; avoid broadening this into a shared helper header unless multiple extracted families prove they need the same interface.
- Storage plans keep narrow local copies of register-bank, register-placement, regalloc lookup, and storage-encoding helpers.
- Call plans keep narrow local copies of call wrapper classification, symbol/callee resolution, memory-return planning, ABI binding lookup, call clobber construction, preserved-value construction, and register/storage helper logic.
- AST dependency checks showed `build_call_clobber_set()` and `build_call_preserved_values()` are still used by runtime-helper publishers, so the extraction intentionally duplicated those helpers locally in `call_plans.cpp`.
- Variadic entry plans keep narrow local copies of value-name lookup, offset alignment, frame-slot id allocation, and variadic storage-slot helpers.
- AST dependency checks showed the atomic publisher chain is self-contained: `publish_contract_plans()` calls `populate_atomic_operations()`, which calls `build_atomic_operation_carrier()`, which depends only on name-table APIs and local atomic missing-fact/value-name helpers.
- AST dependency checks showed the intrinsic publisher chain is self-contained after keeping inline asm on a local value-name helper: `publish_contract_plans()` calls `populate_intrinsic_carriers()`, which calls `find_call_plan_for_instruction()` and `build_intrinsic_carrier()`, and the carrier builder owns the intrinsic validation and missing-fact helpers in `intrinsics.cpp`.
- AST dependency checks showed the inline-asm publisher chain is self-contained after keeping a narrow local value-home lookup in `inline_asm.cpp`: `populate_inline_asm_carriers()` calls `build_inline_asm_carrier()`, which owns inline-asm operand construction, register identity parsing, validation, and missing-fact helpers in the new file.
- AST dependency checks showed the special-carrier publisher chain is self-contained after keeping narrow local copies of `register_bank_from_class()` and `find_storage_plan_value()` in `special_carriers.cpp`: `populate_i128_carriers()` calls `build_i128_carrier()`, `populate_f128_carriers()` calls `build_f128_carrier()`, and the builders own their missing-fact helpers in the new file.
- AST dependency checks showed the f128 runtime-helper prepared fact publisher now has the expected focused dependency surface: `populate_f128_runtime_helper_facts()` calls the moved f128 carrier-binding, ABI-binding, marshaling, boundary-policy, call-ownership, and selected-ownership helpers in `f128_runtime_helpers.cpp`; it reaches prepared carrier/frame/liveness/value-location lookups through existing module APIs.
- The f128 extraction intentionally keeps narrow local copies of `build_call_clobber_set()`, `build_call_preserved_values()`, `preserved_value_has_complete_route()`, `find_call_program_point()`, register placement, and scalar storage helpers.
- AST dependency checks showed the i128 runtime-helper prepared fact publisher now has the expected focused dependency surface: `populate_i128_runtime_helper_lanes()` calls the moved lane-binding, ABI-binding, marshaling, boundary-policy, call-ownership, scalar-ownership, and selected-ownership helpers in `i128_runtime_helpers.cpp`; it reaches prepared carrier/frame/liveness/value-location lookups through existing module APIs.
- The i128 extraction intentionally keeps narrow local copies of `build_call_clobber_set()`, `build_call_preserved_values()`, `preserved_value_has_complete_route()`, `find_call_program_point()`, register placement, liveness/regalloc lookup, and ABI/register-bank helpers.
- AST dependency checks showed `populate_frame_plan()` used only frame-plan-local helper dependencies plus public prepared module APIs and `is_dynamic_alloca_call()`; those helpers now live in `frame_plan.cpp`.
- `prealloc.cpp` is now a coordinator file; keep future additions from reintroducing publisher-specific helper clusters there.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`; build succeeded and all 139 selected backend tests passed. The earlier compile rechecked globbed directories and picked up `frame_plan.cpp`, so no build registration edit was needed. Also ran `git diff --check`; it passed with no whitespace errors. Proof log: `test_after.log`.
