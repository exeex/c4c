Status: Active
Source Idea Path: ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract low-risk standalone fact publishers

# Current Packet

## Just Finished

Completed Step 2 storage-plan fact publisher extraction. `populate_storage_plans()` now lives in `src/backend/prealloc/storage_plans.cpp`, with `src/backend/prealloc/storage_plans.hpp` carrying the narrow publisher declaration. `BirPreAlloc::publish_contract_plans()` includes that header and still calls `populate_storage_plans(prepared_)` in the same phase order after the second frame-plan publish and before carrier/runtime-helper publishers.

## Suggested Next

Extract another low-risk standalone fact publisher from `src/backend/prealloc/prealloc.cpp`; the special-carrier or runtime-helper families should be dependency-checked first because several of them share call, regalloc, and storage-plan lookup helpers.

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
- Larger families to defer: frame plan shares callee-save and placement helpers; call/runtime helpers share `build_call_clobber_set()`, `build_call_preserved_values()`, `find_call_program_point()`, and register/ABI helpers; carriers depend on storage plans and family-specific validation.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`; build succeeded and all 139 selected backend tests passed. Also ran `git diff --check`; it passed with no whitespace errors. Proof log: `test_after.log`.

Post-commit baseline review follow-up: supervisor accepted `test_baseline.new.log` into `test_baseline.log`, so the baseline-review reminder has been handled.
