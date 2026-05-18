# AArch64 Call Argument Register Authority Todo

Status: Active
Source Idea Path: ideas/open/291_aarch64_call_argument_register_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Call Argument Register Authority Failure

# Current Packet

## Just Finished

Localized `src/00210.c` to an AArch64 call-argument register-authority failure
without implementation changes. The failing runtime output is caused by the
first `printf` argument receiving the function pointer register instead of the
format-string register, while the attributed function-pointer calls themselves
remain valid indirect calls.

Evidence from generated assembly
`build-aarch64-scan/c_testsuite_aarch64_backend/src/00210.c.s`:

- `actual_function` is materialized into `x20` and both attributed casts call it
  through `blr x20`; this preserves the function-pointer indirect-call shape.
- `.str0` is materialized into `x21` before each `printf`.
- Each `printf` setup emits `mov x0, x20`, then `mov x1, x13`, then
  `bl printf`; the wrong move is the format argument source, which should use
  the prepared `.str0` source register (`x21`) or an equivalent authoritative
  materialization into ABI argument register `x0`.

Evidence from transient prepared dump `/tmp/00210.callarg.prepared`:

- `@actual_function` storage is `reg=x20`.
- `@.str0` storage is `reg=x21`.
- `printf` callsites record `arg0 bank=gpr from=symbol_address:@.str0 to=x0`.
- The detailed call plan for `arg index=0` records
  `source_symbol=@.str0 source_reg=x21 dest_reg=x0`.
- The before-call move bundle records destination ABI argument placement
  `gpr:call_argument#0/w1 reg=x0` for the same value id.

## Suggested Next

Execute Step 2 by adding focused backend coverage and repairing the call-boundary
source/destination authority path so a prepared symbol-address argument source
such as `.str0` uses its authoritative prepared source register (`x21`) when
moving into ABI argument register `x0`.

Likely backend surfaces:

- `src/backend/mir/aarch64/codegen/calls.cpp` for before-call move lowering,
  prepared argument-plan lookup, and source/destination register selection.
- `src/backend/mir/aarch64/codegen/globals.cpp` for call-argument symbol-address
  materialization records and symbol-address register operands.
- `src/backend/mir/aarch64/codegen/instruction.cpp` only if call-boundary moves
  require def/use or operand-authority plumbing outside the call lowering path.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Do not reopen the closed function-pointer materialization owner unless new
  proof contradicts the closure review.
- Do not reopen the closed scalar parameter/ALU owner unless new proof
  contradicts that closure review.
- Preserve existing attributed function-pointer indirect calls through
  `actual_function`.
- Reject named-case shortcuts for `src/00210.c`, `.str0`, `printf`, one
  symbol, or one emitted move shape.
- Keep the scalar parameter/ALU and function-pointer materialization closures
  separated; this packet only localizes call-argument register authority.

## Proof

Baseline proof was already captured in `test_before.log` with:

`{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00210_c$'; } > test_before.log 2>&1`

Result: `c_testsuite_aarch64_backend_src_00210_c` failed with
`[RUNTIME_MISMATCH]`; expected two lines of `42`, actual output was garbage
bytes consistent with passing a non-format pointer to `printf`.

No proof rerun was needed, so `test_after.log` was not rewritten. Inspection
artifacts were written under `/tmp` only:

- `/tmp/00210.callarg.bir`
- `/tmp/00210.callarg.prepared`
- `/tmp/00210.callarg.main.mir`
