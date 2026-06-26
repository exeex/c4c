Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repeat Family Packets Until 395 Is Exhausted Or Needs Review

# Current Packet

## Just Finished

Step 4 implemented prepared scalar division/remainder instruction-fragment
lowering in `fragment_for_prepared_binary()` for:

- `BinaryOpcode::SDiv` I32/I64 as RV64 M-extension `divw`/`div`.
- `BinaryOpcode::SRem` I32/I64 as `remw`/`rem`.
- `BinaryOpcode::URem` I32/I64 as `remuw`/`remu`.

Existing `UDiv` lowering remains intact. The new cases reuse the prepared
operand materialization and destination stack-home finishing path, so
unsupported widths, missing homes, or non-materializable operands still fail
closed through the existing checks.

Focused backend coverage now builds prepared scalar binary fixtures for all six
new instruction forms and verifies the exact OP-32 versus OP instruction word
split plus the surrounding prepared move/return sequence.

Representative outcomes:
- `src/divcmp-1.c` now passes the delegated allowlisted RV64 gcc-torture object
  probe; it no longer fails on scalar `sdiv`.
- `src/divmod-1.c` still fails with `unsupported_instruction_fragment`, but the
  implemented family is covered for its prepared scalar `sdiv`/`srem`/`urem`
  instruction forms. Its prepared dump also contains additional non-packet
  shapes such as `i8`/`i16` extension/truncation around same-module calls and
  malformed-looking `i16 sret(size=4, align=...)` call-argument renderings, so
  the remaining failure should be isolated before widening Step 4.

## Suggested Next

Delegate the next Step 4 repeat-family packet to isolate the remaining
`src/divmod-1.c` `unsupported_instruction_fragment` after scalar div/rem
lowering. Start from the prepared dump in
`build/agent_state/395_step3_dumps/divmod-1.prepared.txt` and determine whether
the first residual is a prepared small-integer cast/publication shape,
same-module call-argument shape, or another instruction fragment before editing.

## Watchouts

- Do not add filename-specific handling for `divmod-1.c`; the next slice needs
  to identify the semantic residual first.
- Keep stack-frame, parameter-home, and runtime-mismatch ownership out of this
  Step 4 repeat-family packet unless the supervisor explicitly routes those
  ideas.
- The object-route diagnostic for `divmod-1.c` is still generic, so a focused
  minimal prepared fixture or diagnostic probe may be needed before coding.
- Preserve the fixed canonical log path `test_after.log` for executor proof.

## Proof

Ran the delegated proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/395_step4_dumps && printf '%s\n' 'src/divcmp-1.c' 'src/divmod-1.c' > build/agent_state/395_step4_divrem.allowlist.txt && { ALLOWLIST=build/agent_state/395_step4_divrem.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/395_step4_divrem_probe.log 2>&1 || true; } && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|RV64_BACKEND_RUNTIME_MISMATCH|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/395_step4_divrem_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build succeeded; allowlisted probe completed with `total=2 passed=1
failed=1`; `src/divcmp-1.c` passed; `src/divmod-1.c` still failed at
`unsupported_instruction_fragment`; backend CTest passed all selected
`^backend_` tests with `100% tests passed, 0 tests failed out of 326`.
`test_after.log` is the canonical proof log.

Log paths:
- `build/agent_state/395_step4_divrem.allowlist.txt`
- `build/agent_state/395_step4_divrem_probe.log`
- `build/agent_state/395_step3_dumps/divcmp-1.prepared.txt`
- `build/agent_state/395_step3_dumps/divmod-1.prepared.txt`
- `build/rv64_gcc_c_torture_backend/src_divcmp-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_divmod-1.c/case.log`
- `test_after.log`
