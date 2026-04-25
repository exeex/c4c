Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Convert ABI Classification Consumers

# Current Packet

## Just Finished

Step 4 - Convert ABI Classification Consumers converted byval aggregate layout
classification for member aggregate params, returns, and call ABI paths to
prefer the backend structured layout table when `structured_layouts_` is
available, while preserving explicit legacy fallback behavior for static and
fallback-only callers.

Changed files:

- `src/backend/bir/lir_to_bir/aggregate.cpp`
- `src/backend/bir/lir_to_bir/call_abi.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `todo.md`
- `test_after.log`

Converted consumers:

- `lower_byval_aggregate_layout()` now has a structured-table overload that
  uses `lookup_backend_aggregate_type_layout()` and a legacy two-argument
  overload that forwards through the same helper with a null table.
- `collect_aggregate_params()` now classifies member aggregate params with
  `structured_layouts_`.
- Member return classification through `infer_function_return_info()` and
  signature return parsing now passes structured layout state when available.
- Member function parameter lowering now routes through a structured-aware
  `lower_function_params()` wrapper; declaration/static fallback uses
  `lower_function_params_fallback()`.
- Direct, indirect, call-return, and variadic byval aggregate call
  classification in `lower_call_inst()` / `lower_runtime_intrinsic_inst()` now
  pass `structured_layouts_`.

Remaining fallback-only ABI consumers: extern declarations and declaration-only
function lowering still pass null structured layout state explicitly, because
they do not have a member lowerer layout table. The legacy two-argument
`lower_byval_aggregate_layout()` remains for non-ABI fallback-only consumers
outside this packet.

Globals, initializer lowering, memory/addressing.cpp, unrelated MIR files, test
expectations, `plan.md`, and the source idea were not changed.

## Suggested Next

Next coherent packet: send Step 4 to review or delegate the next plan step the
supervisor selects after checking whether the ABI conversion is sufficient.

## Watchouts

`calling.cpp` needed a small ABI call-site update so member call return/arg
classification could pass `structured_layouts_`; it was not listed in Owned
Files but is part of the ABI consumer surface touched by this helper signature.

Do not convert globals, initializer lowering, memory/addressing.cpp, unrelated
MIR files, or test expectations unless the supervisor explicitly widens
ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed, selected ctest subset passed 8/8, and the
backend `c4c_backend` target rebuilt and linked successfully.
