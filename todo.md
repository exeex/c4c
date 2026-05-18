# AArch64 Function Pointer Indirect Call Values Todo

Status: Active
Source Idea Path: ideas/open/289_aarch64_function_pointer_indirect_call_values.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Expand to Global Initializers and Returned Function Pointers

# Current Packet

## Just Finished

Completed the Step 3 structured symbol-spelling hardening packet for the
global/static initializer and returned function-pointer repair.

Supervisor acceptance proof has accepted Step 3 for this function-pointer
owner: the `00089` global/static function-pointer initializer route is
resolved, and the `00124` returned function-pointer route reaches the selected
callee through an indirect call. `00124` is not claimed green because its
remaining runtime-nonzero result is the already separated scalar
parameter/ALU blocker.

Hardened the current implementation so semantic `LinkNameId` spelling is the
authority when available:

- symbol-pointer returns now prefer
  `module.names.link_names.spelling(pointer_symbol_link_name_id)` and only use
  the display name as a compatibility fallback.
- global/static function-pointer initializer emission now prefers
  `module.names.link_names.spelling(initializer_symbol_name_id)` or
  `pointer_symbol_link_name_id` before falling back to stale display text.

Added drift-resistant backend coverage where the display text is intentionally
stale while the `LinkNameId` spelling is authoritative:

- `backend_aarch64_return_lowering` verifies returned symbol-pointer records
  and `.xword <symbol>` global initializer emission use the structured spelling
  rather than stale display strings.
- `backend_aarch64_machine_printer` remains covered for selected
  symbol-pointer returns printing `adrp/add x0` before `ret`.
- `backend_lir_to_bir_notes` remains covered for direct function-pointer return
  values preserving structured `LinkNameId` identity.

The hardened surfaces cover the Step 3 implementation that makes `00089`
preserve indirect calls and initialize the global function-pointer field:

```asm
go:
    adrp x0, anon
    add x0, x0, :lo12:anon
    ret
s:
    .xword zero
```

They also cover the `00124` function-pointer return path that returns the
selected function pointer through the ABI return register and keeps the
caller-side indirect call:

```asm
f1:
    adrp x0, f2
    add x0, x0, :lo12:f2
    ret
main:
    blr x13
```

## Suggested Next

Delegate a separate scalar AArch64 parameter/ALU packet before treating
`src/00124.c` as fully green: `f2` is reached through the returned function
pointer, but its body subtracts stale callee-saved registers instead of the
incoming ABI argument registers.

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
- `src/00089.c` now passes under the delegated proof.
- `src/00124.c` no longer segfaults from a bad returned function-pointer
  callee. The latest delegated proof exits `8`; generated `f1` returns `f2`
  via `x0`, and `main` performs `blr x13`, but generated `f2` still computes
  `sub w0, w19, w20` instead of using its incoming `w0`/`w1` parameters.
  Treat that as a separate scalar parameter/ALU lowering owner unless the
  supervisor intentionally broadens this source idea.
- Do not convert indirect calls to named direct calls; the repaired shape still
  uses `blr`.

## Proof

Built the focused backend test targets:

```sh
cmake --build build --target backend_aarch64_return_lowering_test backend_aarch64_machine_printer_test backend_lir_to_bir_notes_test
```

Result: passed.

Ran the focused backend tests:

```sh
ctest --test-dir build --output-on-failure -R '^(backend_aarch64_return_lowering|backend_aarch64_machine_printer|backend_lir_to_bir_notes)$'
```

Result: passed 3/3.

Ran the delegated backend proof subset:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed 139/139.

Ran the delegated Step 3 owner proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00089_c$|^c_testsuite_aarch64_backend_src_00124_c$'; } > test_after.log 2>&1
```

Result: selected 2 tests. `c_testsuite_aarch64_backend_src_00089_c` passed.
`c_testsuite_aarch64_backend_src_00124_c` failed with `exit=8` after the
function-pointer return/callee path was repaired; `test_after.log` is the
canonical proof log.

Supervisor acceptance proof for the completed Step 3 function-pointer slice:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: passed; resolved `00089` with no new failures.

```sh
cmake --build --preset default
```

Result: passed.

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed 139/139 on clean rerun.

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00(087|089)_c$|^c_testsuite_aarch64_backend_src_00124_c$'; } > /tmp/c4c_fnptr_step3_owner_subset.log 2>&1
```

Result: `00087` passed, `00089` passed, and `00124` failed
runtime-nonzero from the already separated scalar parameter/ALU blocker.
Step 3 is accepted for the function-pointer owner without claiming `00124`
fully green.

Transient inspection artifacts used:
- `/tmp/00089.step3.s`
- `/tmp/00124.step3.s`
