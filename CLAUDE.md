# Step 4 x86 emitter tightening workflow

## Goal

Shrink the remaining x86 emitter-local `lower_lir_to_backend_module(...)` fallback
one bounded LIR family at a time.

## Workflow per slice

1. Read `todo.md` "Next slice" to pick the target family
2. Read the fixture in `tests/backend/backend_test_fixtures.hpp` for the target
   family's `make_*_module()` to understand the LIR shape
3. Add a `parse_minimal_*()` function in `src/backend/x86/codegen/emit.cpp`
   that recognises the bounded LIR pattern and folds it to a constant return
   (or minimal asm)
4. Wire the new parser into `try_emit_direct_lir_module()` — place it before
   the more general parsers so the bounded family is caught first
5. Add a direct-vs-lowered parity test in
   `tests/backend/backend_lir_adapter_x86_64_tests.cpp` that asserts
   `emit_module(lir) == emit_module(lower_lir_to_backend_module(lir))`
6. Register the new test in `main()`
7. Build: `cmake --build build -j8`
8. Run targeted tests: `ctest --test-dir build -R backend_lir_adapter_x86_64 -j1 --output-on-failure`
9. Run full backend suite: `ctest --test-dir build -R backend -j1 --output-on-failure`
   — must stay at 402/402
10. Update `todo.md`: move the family from "Next slice" to "Completed in this slice",
    update "Current active item" and "Recent baseline"

## Acceptance bar

- Must delete or tighten one live production legacy path
- Must not be test-only or probe-only
- If tests change, they prove the production deletion in the same batch
- Backend suite stays monotonic (no regressions)

## Build and test commands

```bash
cmake --build build -j8
ctest --test-dir build -R backend_lir_adapter_x86_64 -j1 --output-on-failure
ctest --test-dir build -R backend -j1 --output-on-failure
ctest --test-dir build -j 8 2>&1 | grep "tests passed"
```

## Key files

- `src/backend/x86/codegen/emit.cpp` — direct LIR parsers and `try_emit_direct_lir_module()`
- `tests/backend/backend_test_fixtures.hpp` — LIR module fixtures
- `tests/backend/backend_lir_adapter_x86_64_tests.cpp` — x86 parity tests
- `todo.md` — slice tracking
