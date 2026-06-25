Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Representative Executable Progress

# Current Packet

## Just Finished

Step 5 repaired the remaining `src/20030216-1.c` RV64 object-route
compile-time unsupported prepared module shape. The prepared module was not a
CFG or edge-copy failure: executable code lowered to `main` returning zero, but
the module still carried a file-scope constant `double one=1.0`, and the RV64
object route rejected every prepared module with globals before building
function text.

RV64 object emission now accepts defined constant scalar globals with immediate
storage and emits them as `.rodata` object symbols in the structured object
model. Mutable globals, extern storage gaps, aggregates, string constants, and
symbol-initialized globals still fail closed. A focused RV64 object-emission
test covers the constant F64 object-storage path while preserving the existing
mutable-global rejection test.

The representative allowlist is now 4 passed and 0 failed:
`src/20000113-1.c`, `src/20000205-1.c`, `src/20010119-1.c`, and
`src/20030216-1.c` all pass the RV64 object-route proof.

## Suggested Next

Supervisor should review and commit this Step 5 code-plus-`todo.md` slice if
the semantic constant-global object route is accepted.

## Watchouts

- `src/20030216-1.c` passes because the object route now handles the prepared
  constant-scalar global storage shape; this is not a filename shortcut or an
  expectation change.
- `src/20000113-1.c` remains passing, preserving the earlier join-transfer
  select-carrier fix.
- `clang-format` is not installed in this container, so the touched C++ files
  were manually style-checked after the failed formatter invocation.
- Keep the route shared-traversal-first; do not scan
  `control_flow.parallel_copy_bundles`, do not use
  `prepared_object_parallel_copy_event_kind` from RV64, and do not add
  filename/test-name shortcuts, expectation weakening, rendered-text probes, or
  target-local CFG reconstruction.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$' && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-multiblock-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true`

Result: build completed, `backend_riscv_object_emission` passed, and the
allowlist is now 4/4 passed and 0/4 failed. `src/20030216-1.c` passes, and the
current three passing cases remain passing. Proof log: `test_after.log`.
