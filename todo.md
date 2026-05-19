Status: Active
Source Idea Path: ideas/open/321_aarch64_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Lower Aggregate Va Arg Helper Output

# Current Packet

## Just Finished

Step 2 replaced raw aggregate `va_arg` helper text with executable AArch64
lowering in `src/backend/mir/aarch64/codegen/variadic.cpp`.

The `VaArgAggregate` printer now fail-closes unless it has a register
`va_list` source, a stack-slot destination payload, at least two scratch
registers, and a supported prepared source class. For supported aggregate
overflow/register-save plans it emits:

- source pointer loads from the prepared `va_list` field
- chunked payload copies using 8-, 4-, and 1-byte transfers based on
  `copy_size_bytes`
- destination stack stores using direct scaled SP offsets when encodable and
  scratch-materialized SP addresses otherwise
- prepared `va_list` progression by `progression_stride_bytes`

`tests/backend/mir/backend_aarch64_machine_printer_test.cpp` now asserts the
aggregate helper output is executable load/store/progression assembly and that
raw `va.arg.aggregate` text is absent from the focused machine-printer output.

## Suggested Next

Classify or repair the new runtime first bad fact: the focused representative
now assembles but segfaults, and generated `myprintf` initializes the local
`va_list` through `x21` before `x21` is materialized as a valid `va_list`
address:

- `mov x0, x21`
- `str x9, [x21]`
- `str x9, [x21, #8]`
- `str x9, [x21, #16]`
- `str w9, [x21, #24]`
- `str w9, [x21, #28]`

The next packet should decide whether that belongs to existing `va_start`
destination-address lowering, frame-slot publication, or a new lifecycle owner.

## Watchouts

- `rg 'va\\.arg\\.aggregate' build/c_testsuite_aarch64_backend/src/00204.c.s`
  returns no matches after this packet.
- The external representative now reaches runtime, so the prior assembler
  blocker is gone.
- The aggregate helper lowering intentionally does not special-case `00204.c`,
  one payload size, one register, one stack slot, or one emitted line.
- The local aggregate helper test covers executable output but uses a small
  stack destination; the external focused representative exercised large stack
  offsets in generated assembly.
- Do not reopen F128 transport addressability; that owner is closed.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  register, one stack slot, one helper index, or one emitted line.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, and 320.
- Keep global initializer emission separate unless it becomes the first bad
  fact after aggregate `va_arg` helper text is gone.

## Proof

Fresh Step 2 proof was written to `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest ran 11 tests, 10 passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed. The failure advanced from
assembler rejection of raw `va.arg.aggregate*` text to
`[RUNTIME_NONZERO] exit=Segmentation fault`.

The focused backend checks, including `backend_aarch64_machine_printer`, passed.
The generated representative assembly no longer contains raw
`va.arg.aggregate`, `va.arg.aggregate.source`, or
`va.arg.aggregate.progress` text.
