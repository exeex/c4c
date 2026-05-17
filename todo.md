Status: Active
Source Idea Path: ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract low-risk standalone fact publishers

# Current Packet

## Just Finished

Completed Step 2 label-identity extraction. `publish_prepared_bir_label_identity()` and its local prepared/BIR block-label resolver now live in `src/backend/prealloc/label_identity.cpp`, with `src/backend/prealloc/label_identity.hpp` carrying the narrow publisher declaration. `BirPreAlloc::publish_contract_plans()` still calls `publish_prepared_bir_label_identity(prepared_)` first, before the rest of the contract-plan publishers.

## Suggested Next

Extract the next low-risk standalone publisher family from `src/backend/prealloc/prealloc.cpp`, preferably another compact family whose dependencies do not force broad helper-header creation.

## Watchouts

- Keep this route behavior-preserving; do not change prepared output, phase order, snapshots, or test expectations.
- Do not split `regalloc.cpp` or `prepared_printer.cpp` under this plan.
- Avoid broad private helper headers that become a second monolith.
- Prefer one fact-publisher family per packet unless dependency edges require grouping.
- Dynamic-stack build registration relies on the existing recursive prealloc source glob; adding `dynamic_stack.cpp` caused CMake to reconfigure successfully during the proof.
- `is_dynamic_alloca_call()` now has package linkage via `dynamic_stack.hpp` because `populate_frame_plan()` still uses it; keep that header narrow.
- Label-identity build registration also relies on the existing recursive prealloc source glob; adding `label_identity.cpp` caused CMake to reconfigure successfully during the proof.
- `find_preferred_block_label_id()` remains duplicated narrowly in `dynamic_stack.cpp` and `label_identity.cpp`; do not introduce a broad shared helper header unless more extracted publishers need it.
- Larger families to defer: frame plan shares callee-save and placement helpers; call/runtime helpers share `build_call_clobber_set()`, `build_call_preserved_values()`, `find_call_program_point()`, and register/ABI helpers; carriers depend on storage plans and family-specific validation.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`; build succeeded and all 139 selected backend tests passed. Also ran `git diff --check`; it passed with no whitespace errors. Proof log: `test_after.log`.

Post-commit baseline review follow-up: supervisor accepted `test_baseline.new.log` into `test_baseline.log`, so the baseline-review reminder has been handled.
