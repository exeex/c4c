Status: Active
Source Idea Path: ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract special carriers and runtime helper publishers

# Current Packet

## Just Finished

Completed Step 4 intrinsic prepared fact publisher extraction. `populate_intrinsic_carriers()` now lives in `src/backend/prealloc/intrinsics.cpp`, with `src/backend/prealloc/intrinsics.hpp` carrying the narrow publisher declaration. `BirPreAlloc::publish_contract_plans()` includes that header and still calls `populate_intrinsic_carriers(prepared_)` in the same phase order after atomic publication and before inline-asm publication. Intrinsic value-name/home lookup, prepared call-plan lookup, carrier construction, family validation, and missing-fact bookkeeping were moved into the focused publisher. Inline asm keeps a narrow local value-name helper in `prealloc.cpp`; no broad private helper header was added. The existing recursive prealloc source glob picked up `intrinsics.cpp`; no build registration edit was needed.

## Suggested Next

Extract the next coherent Step 4 runtime-helper publisher family only after dependency-checking whether its helpers are still shared with adjacent carrier publication; avoid growing a broad private helper header.

## Watchouts

- Keep this route behavior-preserving; do not change prepared output, phase order, snapshots, or test expectations.
- Do not split `regalloc.cpp` or `prepared_printer.cpp` under this plan.
- Avoid broad private helper headers that become a second monolith.
- Prefer one fact-publisher family per packet unless dependency edges require grouping.
- Dynamic-stack build registration relies on the existing recursive prealloc source glob; adding `dynamic_stack.cpp` caused CMake to reconfigure successfully during the proof.
- Storage-plan build registration also relies on the existing recursive prealloc source glob; adding `storage_plans.cpp` caused CMake to reconfigure successfully during the proof.
- `is_dynamic_alloca_call()` now has package linkage via `dynamic_stack.hpp` because `populate_frame_plan()` still uses it; keep that header narrow.
- Label-identity build registration also relies on the existing recursive prealloc source glob; adding `label_identity.cpp` caused CMake to reconfigure successfully during the proof.
- `find_preferred_block_label_id()` remains duplicated narrowly in `dynamic_stack.cpp` and `label_identity.cpp`; do not introduce a broad shared helper header unless more extracted publishers need it.
- Regalloc placement identity keeps a narrow local copy of its placement/spill-slot/value-id helpers in `regalloc_placement_identity.cpp`; the original anonymous-namespace helpers remain in `prealloc.cpp` for other publisher families, so avoid broadening this into a shared helper header unless multiple extracted families prove they need the same seam.
- Storage plans keep narrow local copies of register-bank, register-placement, regalloc lookup, and storage-encoding helpers; `prealloc.cpp` keeps its own versions where other publisher families still need them.
- Call plans keep narrow local copies of call wrapper classification, symbol/callee resolution, memory-return planning, ABI binding lookup, call clobber construction, preserved-value construction, and register/storage helper logic. `prealloc.cpp` keeps its own copies of still-shared frame/runtime-helper helpers rather than growing a broad private helper header.
- AST dependency checks showed `build_call_clobber_set()` and `build_call_preserved_values()` are still used by runtime-helper publishers, so the extraction intentionally duplicated those helpers locally in `call_plans.cpp` while preserving the `prealloc.cpp` copies for current users.
- Variadic entry plans keep narrow local copies of value-name lookup, offset alignment, frame-slot id allocation, and variadic storage-slot helpers; `prealloc.cpp` keeps its own copies where frame and runtime-helper publishers still need them.
- AST dependency checks showed the atomic publisher chain is self-contained: `publish_contract_plans()` calls `populate_atomic_operations()`, which calls `build_atomic_operation_carrier()`, which depends only on name-table APIs and local atomic missing-fact/value-name helpers.
- AST dependency checks showed the intrinsic publisher chain is self-contained after keeping inline asm on a local value-name helper: `publish_contract_plans()` calls `populate_intrinsic_carriers()`, which calls `find_call_plan_for_instruction()` and `build_intrinsic_carrier()`, and the carrier builder owns the intrinsic validation and missing-fact helpers in `intrinsics.cpp`.
- Larger families to defer: frame plan shares callee-save and placement helpers; call/runtime helpers share `build_call_clobber_set()`, `build_call_preserved_values()`, `find_call_program_point()`, and register/ABI helpers; carriers depend on storage plans and family-specific validation.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`; build succeeded and all 139 selected backend tests passed. A prior build reconfigured because the recursive prealloc source glob picked up `intrinsics.cpp`. Also ran `git diff --check`; it passed with no whitespace errors. Proof log: `test_after.log`.

Post-commit baseline review follow-up: supervisor accepted `test_baseline.new.log` into `test_baseline.log`, so the baseline-review reminder has been handled.
