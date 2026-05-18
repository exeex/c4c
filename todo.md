# AArch64 Backend Non-Leaf Call-Frame LR Preservation Todo

Status: Active
Source Idea Path: ideas/open/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Backend Contract Coverage

# Current Packet

## Just Finished

Completed plan.md Step 1, "Confirm Non-Leaf Failure Shape", as an
investigation-only packet. The three existing AArch64 scan artifacts all show
non-leaf `main` functions that call through `bl` and later return with a bare
`ret`, with no visible `x30` save/restore or frame boundary around the call:

- `00100.c.s`: `main` emits `bl foo`, moves `x0` to `x13`, then `ret`.
- `00116.c.s`: `main` emits `bl f`, moves `x0` to `x13`, then `ret`.
- `00121.c.s`: `main` emits `bl f`, moves `x0` to `x19`, emits `bl g`, moves
  `x0` to `x13`, computes `sub w0, w19, w13`, then `ret`.

The source probes are ordinary same-module direct-call returns:
`00100.c` returns `foo()`, `00116.c` returns `f(0)`, and `00121.c` returns
`f(1) - g(1)`. The failure owner is therefore the general non-leaf
call/return/frame rule, not a c-testsuite filename or text-shape case.

## Suggested Next

Execute plan.md Step 2 by adding focused backend contract coverage before the
repair. The likely first test surface is
`tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp` for the
call/return target-MIR contract, with `backend_aarch64_machine_printer_test.cpp`
and `backend_aarch64_return_lowering_test.cpp` as nearby patterns if the slice
needs printer or traversal-level coverage. Register the test through the
existing `tests/backend/mir/CMakeLists.txt` AArch64 CPP test pattern only if a
new test file is cleaner than extending an existing one.

## Watchouts

- Do not special-case c-testsuite filenames, function names, exact emitted text,
  timeout settings, allowlists, unsupported classifications, or expected
  outputs.
- Keep printf, string literals, variadic calls, loop predicates,
  short-circuiting, aggregates/pointers, static globals, and goto behavior out
  of the first LR preservation repair.
- Use explicit timeouts for runtime probes and check for stale generated
  runtime processes after timeout-oriented runs.
- Owning implementation surfaces found by AST-backed/local inspection:
  `src/backend/mir/aarch64/codegen/calls.cpp` lowers calls through
  `lower_prepared_call_instruction()` and `make_call_instruction()`, including
  prepared clobber/preserved effect snapshots; `returns.cpp` lowers bare return
  nodes through `lower_prepared_return_terminator()` and
  `make_return_instruction()`; `prologue.cpp` inserts frame setup/teardown via
  `insert_prepared_frame_boundary_nodes()`, but currently only for simple fixed
  frames and explicitly rejects functions with saved callee registers; and
  `machine_printer.cpp::print_frame()` currently rejects non-empty
  `saved_callee_registers`.
- The AAPCS64 contract says `x30` is link-register-owned, calls clobber or
  define return-control state through `x30`, and return blocks consume that
  state. It also notes there is no dedicated return-control carrier today, so
  Step 2 should pin the missing contract without inventing testcase-shaped
  matching.
- Nearby existing tests already cover `link_register()` classification in
  `backend_aarch64_register_vocabulary_test.cpp`, call clobber/preserve effect
  projection in `backend_aarch64_target_instruction_records_test.cpp`, and
  return-node lowering in `backend_aarch64_return_lowering_test.cpp`. No current
  focused test asserts that a selected non-leaf call/return path preserves LR.

## Proof

Investigation-only packet. Inspected existing generated assembly under
`build-aarch64-scan/c_testsuite_aarch64_backend/src/{00100,00116,00121}.c.s`,
the matching c-testsuite sources, AST-backed symbol inventories for
`prologue.cpp`, `returns.cpp`, and `calls.cpp`, and nearby AArch64 backend test
patterns. No build, CTest, runtime execution, implementation edit, test edit,
or root proof log was required or produced.

Proposed narrow proof command for the first code-changing slice:

```sh
cmake --build build --target backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_return_lowering_test && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|return_lowering)$' --output-on-failure
```

If the slice also regenerates/checks the three public AArch64 probes after the
backend contract repair, add:

```sh
ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_00(100|116|121)_c$' --output-on-failure --timeout 10
```
