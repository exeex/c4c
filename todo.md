Status: Active
Source Idea Path: ideas/open/313_rv64_external_empty_stub_policy_runtime.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Stop Emitting Executable Empty External Bodies

# Current Packet

## Just Finished

Step 3 declaration/definition separation for RV64 prepared module emission.

RV64 prepared module emission now resolves the matching non-declaration BIR
function before publishing a global label. Declaration-only control-flow
records are skipped, so external calls remain `call <symbol>` relocations
instead of resolving to compiler-owned empty bodies. Same translation-unit
definitions still publish `.globl name` and `name:`.

The three Step 2 expected-repair tests were flipped to ordinary passing
contracts:

- `backend_codegen_route_riscv64_external_strlen_runtime_link_policy`
- `backend_rv64_runtime_riscv64_external_strlen_runtime_link_policy`
- `backend_codegen_route_riscv64_external_stdio_declaration_stub_guard`

The fixed-arity `strlen` case needed one adjacent ordinary scalar tail repair:
prepared RV64 casts now support named-result integer `trunc`, allowing
`trunc i64` call results to flow into the final subtract and return path.

## Suggested Next

Proceed to the next external-call policy packet: classify and repair the
remaining supported external-call residuals without reintroducing executable
declaration bodies. A likely next boundary is variadic `printf`/stdio call
handling, because `00056`, `00125`, and `00179` now emit and link without local
declaration labels but still exit under qemu with illegal instruction status
132.

## Watchouts

- Do not add fake named libc/libm/string/user external bodies.
- Do not classify every external call as unsupported unless supported runtime
  linkage has been considered and preserved where available.
- The stdio declaration guard is green, but it still does not prove variadic
  `printf` execution.
- `src/00025.c` now passes emit, clang, and qemu. `src/00056.c`,
  `src/00125.c`, and `src/00179.c` all emit/link and have no local external
  declaration labels, but still exit 132 under qemu.
- Keep same-TU definitions publishing real labels; the policy fix is only for
  declaration-only functions.

## Proof

Ran:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(codegen_route|rv64_runtime)_riscv64_external_(strlen_runtime_link_policy|stdio_declaration_stub_guard)'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Results:

- Build passed.
- Focused external-policy subset passed `3/3`.
- Backend guard passed against `test_before.log`: before `229 passed, 1 failed,
  230 total`; after `232 passed, 1 failed, 233 total`; new failures `0`.
- Existing `backend_riscv_prepared_edge_publication` remains the only backend
  subset failure.
- Runtime probe artifacts:
  `build/rv64_c_testsuite_probe_latest/triage_313_step3/probe_results.tsv`.
  `00025` passed; `00056`, `00125`, and `00179` remained qemu `132` after
  declaration-label separation.
