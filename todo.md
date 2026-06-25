Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 baseline-review cleanup for the stale RV64 CLI
helper-free variadic entry expected-failure test. The test now uses the passing
object-codegen harness because fact-complete helper-free RV64 variadic entries
are intentionally admitted, while preserving the same source case, test name,
output object path, and labels.

Changed files:
- `tests/backend/CMakeLists.txt`
- `todo.md`

## Suggested Next

Return to the Step 4 FPR ABI edge route by running the representative
`src/20030125-1.c` validation to confirm the old `bir.fpext` boundary advances.
If it exposes the expected `bir.fptrunc double -> float` FPR-register boundary,
the next coherent implementation packet is prepared FPR `fptrunc` lowering or a
deliberately narrow structured diagnostic, depending on the supervisor's route
choice.

Suggested representative proof after the focused slice:

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

## Watchouts

- This packet is baseline-review cleanup only. Do not count the CMake harness
  conversion as a new feature slice.
- The `backend_cli_riscv64_variadic_entry_missing_contract_obj` name and
  `missing_contract` label are intentionally preserved to minimize CTest label
  churn around the historical source case.
- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- This packet intentionally did not implement `fptrunc`, floating calls, libm
  behavior, or stack-slot FPR materialization.
- The FPR cast lowerer requires `PreparedTargetRegisterIdentity` to identify
  RV64 FPR physical registers; it does not infer FPR numbers from names alone.
- The local-memory address predicate remains intentionally narrow: default
  address space, non-volatile, frame-slot base, published frame-slot id,
  base-plus-offset eligibility, matching access size, compatible alignment,
  non-negative offset, and signed 12-bit immediate reach.
- Narrow local loads use the same stack-load helper and therefore encode signed
  `lb`/`lh`; add an explicit load fixture before changing signedness behavior.
- `src/20001203-1.c` is now green in the representative harness; do not keep
  expanding this packet around that testcase.
- `src/20030125-1.c` remains out of scope for this packet and should still be
  classified separately before any FPR or floating-operation implementation.

## Proof

Delegated proof:

```sh
ctest --test-dir build -R '^backend_cli_riscv64_variadic_entry_missing_contract_obj$' --output-on-failure > test_after.log
```

Result: passed, with 1/1
`backend_cli_riscv64_variadic_entry_missing_contract_obj` test passing.

Proof log: `test_after.log`.
