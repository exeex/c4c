Status: Active
Source Idea Path: ideas/open/313_rv64_external_empty_stub_policy_runtime.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Unsupported External Diagnostics

# Current Packet

## Just Finished

Step 4 unsupported-external diagnostics for RV64 prepared module emission.

RV64 prepared module emission now preflights direct external call plans before
publishing a function label. Direct external call shapes the simple RV64 emitter
does not support, including declaration-only variadic `printf` and arbitrary
user-declared unresolved externals, throw an `unsupported_external_call`
diagnostic instead of producing partial runnable assembly. The current RV64
supported-runtime allowlist is intentionally narrow: `strlen` remains the
preserved fixed-arity runtime-linked call, and its route/runtime policy tests
remain ordinary passing contracts that still forbid local `strlen:` /
`.globl strlen` bodies.

The stdio declaration guard was converted from a positive route test to an
expected-failure diagnostic contract, and a focused user-declared unresolved
external case was added:

- `backend_codegen_route_riscv64_external_stdio_unsupported_variadic_diagnostic`
- `backend_codegen_route_riscv64_external_user_unresolved_diagnostic`

## Suggested Next

Proceed to the next external-call policy packet by either broadening the
supported runtime external surface with explicit link/runtime proof for each
symbol family, or by probing the remaining candidate bucket under the current
`strlen`-only allowlist. Do not promote additional libc/libm/string/user
externs to supported without focused tests.

## Watchouts

- Do not add fake named libc/libm/string/user external bodies.
- Do not classify every external call as unsupported; `strlen` fixed-arity
  runtime linkage remains supported for the current scalar-GPR RV64 path.
- Do not allow arbitrary scalar-GPR fixed-arity externs through as runnable
  assembly without an explicit supported-runtime policy entry and proof.
- The stdio declaration guard now proves the diagnostic boundary only; it does
  not implement variadic `printf` execution.
- `src/00025.c` now passes emit, clang, and qemu. `src/00056.c`,
  `src/00125.c`, and `src/00179.c` contain variadic stdio calls and should not
  be treated as supported until RV64 variadic external ABI lowering exists.
- Keep same-TU definitions publishing real labels; the policy fix is only for
  declaration-only functions.

## Proof

Ran:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(codegen_route|rv64_runtime)_riscv64_external_(strlen_runtime_link_policy|stdio_unsupported_variadic_diagnostic|user_unresolved_diagnostic)'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Results:

- Build passed.
- Focused external-policy subset passed `4/4`.
- Delegated backend proof wrote `test_after.log` and failed with the existing
  `backend_riscv_prepared_edge_publication` failure as the only failing
  backend test.
