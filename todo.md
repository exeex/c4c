Status: Active
Source Idea Path: ideas/open/278_aarch64_backend_local_operand_materialization_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Local Operand Materialization

# Current Packet

## Just Finished

Step 2 local-materialization repair is complete, including the reviewer-found
immediate-store support boundary. AArch64 local stores now select immediate
store records from authoritative prepared frame-slot address facts,
unstructured local load/store instructions may use prepared size/offset facts,
and immediate store materialization uses valid `movz`/`movk` sequences in the
reserved MIR scratch pool (`w9`/`x9`) instead of indirect-call/linker scratch
registers. Local loads then feed the prepared destination register before
scalar ALU emission.

Changed files:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
- `todo.md`
- `test_after.log`

Generated `00003.c` AArch64 now materializes the local `x = 4`, stores it to
the prepared frame slot, reloads it into the prepared `%t0` home (`w13`), and
then performs the subtraction:

```asm
main:
    movz w9, #4
    str w9, [sp]
    ldr w13, [sp]
    sub w0, w13, #4
    ret
```

Focused memory coverage proves unstructured prepared local immediate stores
select without fabricating stored value homes and print through the reserved MIR
scratch register (`w9`). It now uses the non-trivial constant `123456789` and
expects `movz w9, #52501` plus `movk w9, #1883, lsl #16` before the `str`, so
the selected immediate-store subset no longer claims a single invalid
`mov #imm` can materialize arbitrary constants. The runtime smoke route for
`00001.c`, `00002.c`, and `00003.c` passes.

## Suggested Next

Step 2 is acceptance-ready for supervisor review. A coherent follow-up packet
would broaden AArch64 backend scan coverage after the local materialization,
using the supervisor-selected broader route rather than adding more named smoke
cases here.

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, or CTest contracts.
- Do not add filename-specific handling for `00003.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Preserve the idea 277 result-register repair: returned integer expression
  values must remain in `w0`/`x0` before `ret`.
- Keep runtime-runner and route-diagnostic work out of this plan unless the
  supervisor opens a separate lifecycle item.
- Keep ABI classification helpers such as `caller_saved_gp_registers()` as ABI
  vocabulary; prepared structured placement conversion now has its own
  prealloc-aligned caller-saved mapping.
- The immediate-store printer currently covers the selected 4-byte and 8-byte
  integer store subset with `movz`/`movk` scratch materialization. Smaller
  local widths should remain fail-closed until byte/halfword store mnemonics
  are modeled.
- Do not borrow `x16`/`x17` for generic materialization; those remain
  indirect-call/linker scratch registers. Use reserved MIR scratch (`x9`/`x10`)
  for this class of printer materialization.

## Proof

Ran the delegated proof:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_register_vocabulary|backend_aarch64_prepared_register_conversion|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_operand_resolution|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003)_c$'; } 2>&1 | tee test_after.log
```

Result: command exited `0`. All nine backend tests in the delegated subset
passed. `c_testsuite_aarch64_backend_src_00001_c`,
`c_testsuite_aarch64_backend_src_00002_c`, and
`c_testsuite_aarch64_backend_src_00003_c` all passed through the AArch64
backend runtime route. Proof log: `test_after.log`.
