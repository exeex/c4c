Status: Active
Source Idea Path: ideas/open/313_rv64_external_empty_stub_policy_runtime.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize External Call Policy Evidence

# Current Packet

## Just Finished

Step 1 evidence normalization for `src/00025.c`, `src/00056.c`,
`src/00125.c`, `src/00179.c`, and `src/00220.c`.

Scratch path:
`build/rv64_c_testsuite_probe_latest/triage_313_step1/`.

Probe results:

| case | emit | clang | qemu | classification |
| --- | ---: | ---: | ---: | --- |
| `src/00025.c` | 0 | 0 | 124 | supported-linked external policy gap |
| `src/00056.c` | 0 | 0 | 132 | supported variadic external plus declaration-stub gap |
| `src/00125.c` | 0 | 0 | 132 | supported variadic external plus declaration-stub gap |
| `src/00179.c` | 0 | 0 | 132 | mixed string runtime plus declaration-stub gap |
| `src/00220.c` | 0 | 1 | - | non-policy residual first, with declaration stubs present |

Representative evidence:

- `00025` has a prepared direct extern fixed-arity `strlen` call plan and emits
  `call strlen`, but the same assembly also emits a local `.globl strlen` /
  `strlen:` label after `main`. `nm` reports `strlen` as a defined text symbol,
  so the call resolves to the compiler-owned empty label instead of libc.
- `00056` and `00125` have prepared `printf` calls with
  `wrapper_kind=direct_extern_variadic`. RV64 prepared call emission rejects
  variadic wrappers today, truncates `main` before the call, and then falls into
  bodyless header declaration labels such as `printf`, `ftrylockfile`, and
  `__overflow`.
- `00179` has many string and stdio extern calls planned. It is useful for later
  supported string coverage, but the first emitted file is dominated by the
  same declaration-label policy failure plus broader string/variadic scope.
- `00220` fails clang first with `Undefined temporary symbol .Lmain_block_2`;
  it also contains bodyless stdio/wchar declaration labels, but its first
  failure is a separate loop/control/local-data residual.

Bodyless labels are introduced in
`src/backend/mir/riscv/codegen/prepared_module_emit.cpp`: the module emitter
prints `.globl <function>` and `<function>:` for every prepared control-flow
function before checking whether the matching BIR function is only a
declaration. `append_simple_prepared_bir_function_asm` then returns true for
`function.is_declaration`, leaving the empty executable label behind.

Supported/unsupported split:

- Supported-linked externals: runtime-provided libc/string calls such as
  `strlen`, `printf`, and common string/memory functions should lower to
  external relocations with no compiler-owned body labels.
- Unsupported diagnostics: not selected from this packet. The five
  representatives do not isolate a genuine unavailable user external or
  unavailable runtime surface, so blanket unsupported policy would be route
  drift.
- Separate residuals: `00220` needs ordinary loop/control/local-data repair
  before it can prove this policy; `00179` should be kept as broader string
  coverage after the declaration-label boundary.

## Suggested Next

Add Step 2 policy coverage for the selected first repair boundary: declaration
functions must not emit executable local body labels. Use a small fixed-arity
`strlen` runtime-link case as the first positive test and a small
stdio/`printf` header case as a declaration-stub guard. Keep variadic call
execution and unsupported user-extern diagnostics as follow-up boundaries unless
the coverage can isolate them cleanly.

## Watchouts

- Do not add fake named libc/libm/string/user external bodies.
- Do not classify every external call as unsupported unless supported runtime
  linkage has been considered and preserved where available.
- Reject progress that only removes one observed empty label while bodyless
  executable external stubs remain elsewhere.
- Keep ordinary control, local address, aggregate, and function-pointer work out
  of this route unless it is needed only to isolate an external-call policy
  test.
- The first semantic boundary is declaration/definition separation in RV64
  prepared module emission, not variadic `printf` execution. `00056`/`00125`
  should not be claimed green until variadic call lowering is handled.
- `00220` is not a policy proof target yet because `.Lmain_block_2` fails before
  external linkage can be observed.

## Proof

Evidence-only packet. Ran emit, clang/link, qemu, `--dump-bir`,
`--dump-prepared-bir`, `nm`, and `objdump` probes under
`build/rv64_c_testsuite_probe_latest/triage_313_step1/`. No build or CTest run
was required, and `test_after.log` was not produced or modified.
