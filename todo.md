# AArch64 Function Pointer Indirect Call Values Todo

Status: Active
Source Idea Path: ideas/open/289_aarch64_function_pointer_indirect_call_values.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Attributed Function-Pointer Cast Overlap

# Current Packet

## Just Finished

Executed Step 4, "Validate Attributed Function-Pointer Cast Overlap", against
`tests/c/external/c-testsuite/src/00210.c`.

No function-pointer value/callee loss remains in this case. Semantic BIR lowers
both attributed casts to indirect calls through `@actual_function`, prepared
call plans preserve the indirect callee in `x20`, and generated AArch64 uses
`blr x20` for both calls:

```asm
adrp x20, actual_function
add x20, x20, :lo12:actual_function
blr x20
...
adrp x20, actual_function
add x20, x20, :lo12:actual_function
blr x20
```

The remaining `00210` mismatch is a separate backend call-argument/register
authority issue. Prepared BIR says each `printf` receives the string address as
argument 0:

```text
arg0 bank=gpr from=symbol_address:@.str0 to=x0
storage @.str0 register bank=gpr placement=gpr:callee_saved#1/w1 reg=x21
```

Generated AArch64 materializes `.str0` into `x21`, but the following
call-boundary move passes `x20` as `printf`'s format argument:

```asm
adrp x21, .str0
add x21, x21, :lo12:.str0
mov x0, x20
mov x1, x13
bl printf
```

The mismatch is explained by the AArch64 prepared-register conversion surface:
`callee_saved#1` maps to `x20` in the current conversion helper, while the
prepared call/storage facts for this function name the authoritative register
as `x21`. That register placement/spelling disagreement affects the `printf`
format pointer after the function-pointer calls are already correct.

## Suggested Next

Delegate a separate AArch64 prepared-register call-boundary packet for
symbol-address call arguments: when prepared storage names an explicit source
register, call-argument moves should not let a conflicting placement-derived
callee-saved register override that spelling.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Reject named-case shortcuts, exact function-name shortcuts, aggregate-field
  shortcuts, and emitted-instruction-text matching.
- Do not treat this as residual stack-frame/SP alignment unless a new
  unaligned frame is proven.
- Preserve direct-call behavior and existing aligned frame behavior.
- Keep non-bus runtime failures from the wider sample outside this route unless
  they are proven to share the same function-pointer indirect-call owner.
- `src/00089.c` remains accepted for the Step 3 function-pointer owner.
- `src/00124.c` remains separated on the scalar parameter/ALU blocker:
  generated `f1` returns `f2` via `x0`, and `main` performs `blr x13`, but
  generated `f2` still computes with stale callee-saved registers instead of
  incoming `w0`/`w1` parameters.
- `src/00210.c` is blocked outside the function-pointer owner by a general
  AArch64 call-argument register-authority mismatch for the `printf` format
  string: prepared facts say `.str0` is in `x21`, but the emitted move uses
  `x20`.
- Do not convert indirect calls to named direct calls; the repaired shape still
  uses `blr`.

## Proof

Inspected `00210` with semantic and prepared backend dumps:

```sh
build-aarch64-scan/c4cll --target aarch64-linux-gnu --dump-bir tests/c/external/c-testsuite/src/00210.c
build-aarch64-scan/c4cll --target aarch64-linux-gnu --dump-prepared-bir tests/c/external/c-testsuite/src/00210.c
```

Result: semantic BIR and prepared call plans preserve the attributed
function-pointer casts as callable indirect callees. The remaining mismatch is
the prepared-register call-boundary issue described above.

Ran the delegated Step 4 proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00210_c$'; } > test_after.log 2>&1
```

Result: selected 1 test. `c_testsuite_aarch64_backend_src_00210_c` failed with
`[RUNTIME_MISMATCH]`: expected two `42` lines, actual output is garbage bytes
from passing the function address as `printf`'s format pointer. `test_after.log`
is the canonical proof log for this Step 4 packet.

Transient inspection artifacts used:
- `/tmp/00210.bir`
- `/tmp/00210.prepared`
- `/tmp/00210.mir`
