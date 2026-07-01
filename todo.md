Status: Active
Source Idea Path: ideas/open/443_rv64_prepared_value_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Ordinary-C Coverage

# Current Packet

## Just Finished

Completed `plan.md` Step 4 ordinary-C coverage for the repaired prepared-value
StoreGlobal object-emission path.

Added `tests/backend/case/riscv64_global_store_prepared_value_preserves_source.c`,
an ordinary-C case that computes a non-immediate scalar value, stores it through
a global, reloads that global, and compares the stored value against the local
producer result. Registered both focused RV64 runtime and RV64 object-runtime
coverage:

- `backend_rv64_runtime_riscv64_global_store_prepared_value_preserves_source`
- `backend_obj_runtime_rv64_global_store_prepared_value_preserves_source`

## Suggested Next

Proceed to Step 5 validation and handoff with fresh representative proof for
`src/pr81503.c`, the focused coverage added in Step 4, and the
supervisor-selected regression subset.

## Watchouts

- The text assembly route still fails with `riscv prepared module emitter does
  not support this prepared global storage layout`, so object-route evidence is
  the useful proof for `pr81503.c`.
- Step 4 did not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- The new coverage uses the existing object-runtime path to exercise the fixed
  object-emission global-store value preservation behavior.

## Proof

Ran the delegated proof command exactly. It passed:

- `cmake --build build --target c4cll`
- `ctest --test-dir build -j --output-on-failure -R "backend_(obj_runtime_)?rv64_.*global_store_prepared_value_preserves_source"`
  passed 2/2
- `git diff --check -- todo.md tests/backend/CMakeLists.txt tests/backend/case`
- `ctest --test-dir build -j --output-on-failure -R "^backend_"`
  passed 330/330

`test_after.log` is preserved.
