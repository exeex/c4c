Status: Active
Source Idea Path: ideas/open/313_rv64_external_empty_stub_policy_runtime.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Stop Emitting Executable Empty External Bodies

# Current Packet

## Just Finished

Step 2 policy coverage for RV64 declaration-only external functions.

Added compact semantic backend cases:

- `tests/backend/case/riscv64_external_strlen_runtime_link.c`
- `tests/backend/case/riscv64_external_stdio_declaration_guard.c`

Added expected-repair tests:

- `backend_codegen_route_riscv64_external_strlen_runtime_link_policy`
  requires `call strlen` and forbids local `strlen:` / `.globl strlen` body
  publication. It is marked `WILL_FAIL` until declaration-only functions stop
  emitting executable local labels.
- `backend_rv64_runtime_riscv64_external_strlen_runtime_link_policy`
  requires the same fixed-arity external `strlen` case to link through the
  target runtime and exit 0 under qemu. It is marked `WILL_FAIL` until the call
  resolves to libc instead of the compiler-owned empty `strlen:` label.
- `backend_codegen_route_riscv64_external_stdio_declaration_stub_guard`
  verifies a small `stdio`/`printf` include shape does not emit local body
  labels for declaration-only functions such as `printf`, `remove`,
  `ftrylockfile`, and `__overflow`. It is marked `WILL_FAIL`; variadic
  `printf` execution remains outside this first policy contract.

CTest log confirmation:

- The `strlen` route contract currently fails on forbidden `strlen:`.
- The `stdio` guard currently fails on forbidden `printf:` and also shows the
  broader header declaration-label family.
- The `strlen` runtime contract currently times out under qemu because the call
  resolves to the compiler-owned empty label.

## Suggested Next

Implement Step 3 declaration/definition separation in RV64 prepared module
emission. Remove the expected-failure properties from the three new
external-policy tests when the assembly stops emitting declaration-only body
labels and the fixed-arity `strlen` runtime case exits 0.

## Watchouts

- Do not add fake named libc/libm/string/user external bodies.
- Do not classify every external call as unsupported unless supported runtime
  linkage has been considered and preserved where available.
- Reject progress that only removes one observed empty label while bodyless
  executable external stubs remain elsewhere.
- The stdio guard intentionally does not require variadic `printf` execution to
  pass; it is only a declaration-stub policy guard for the next packet.
- A real same-translation-unit function definition must still emit `.globl name`
  and `name:` when needed by normal calls.

## Proof

Ran:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(codegen_route|rv64_runtime)_riscv64_external_(strlen_runtime_link_policy|stdio_declaration_stub_guard)'`

Result: build passed and the focused subset passed `3/3` via explicit
`WILL_FAIL` properties on the three new post-repair contracts. No broad backend
guard was requested, and `test_after.log` was not produced or modified.
