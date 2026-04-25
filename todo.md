Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert ABI Classification Consumers

# Current Packet

## Just Finished

Step 4 - Convert ABI Classification Consumers converted byval aggregate layout
classification for the remaining local aggregate load/store and CFG phi
planning consumers to prefer the backend structured layout table when member
state is available, while preserving explicit fallback behavior.

Changed files:

- `src/backend/bir/lir_to_bir/cfg.cpp`
- `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `todo.md`
- `test_after.log`

Converted consumers:

- `lower_memory_store_inst()` now classifies aggregate stores through
  `lower_byval_aggregate_layout(..., &structured_layouts_)`.
- `lower_memory_load_inst()` now classifies aggregate loads through
  `lower_byval_aggregate_layout(..., &structured_layouts_)`.
- `collect_phi_lowering_plans()` now classifies aggregate phi plans through
  `lower_byval_aggregate_layout(..., &structured_layouts_)`.
- `lower_intrinsic_aggregate_layout()` keeps the explicit null-table fallback
  and delegates the structured-aware path to `lower_byval_aggregate_layout()`
  instead of duplicating layout validation.

Remaining fallback-only ABI consumers: extern declarations, declaration-only
function lowering, and other static helper entry points that lack member
`structured_layouts_` state still use the explicit null-table or legacy
two-argument fallback path.

Globals, initializer lowering, memory/addressing.cpp, unrelated MIR files, test
expectations, `plan.md`, and the source idea were not changed.

## Suggested Next

Next coherent packet: send Step 4 to review, or delegate the next plan step the
supervisor selects after confirming this completes the structured-aware ABI
consumer conversion.

## Watchouts

Do not convert globals, initializer lowering, memory/addressing.cpp, unrelated
MIR files, or test expectations unless the supervisor explicitly widens
ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed, selected ctest subset passed 8/8, and the
backend `c4c_backend` target rebuilt and linked successfully.
