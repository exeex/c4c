Status: Active
Source Idea Path: ideas/open/354_aarch64_external_call_symbol_home_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The 00187 Call/Symbol-Home Boundary

# Current Packet

## Just Finished

Step 1 localized the current `c_testsuite_aarch64_backend_src_00187_c`
segfault to preserved stack-home publication for string-symbol call operands.
Prepared BIR assigns reusable string constants to stack homes, but generated
AArch64 later reloads those homes before external calls without publishing the
symbol addresses into the stack slots.

Representative first bad boundary:

- Prepared BIR for the second `fopen` has callsite `block=0 inst=6`,
  `callee=fopen`, `arg0 from=symbol_address:@.str0 to=x0`, and
  `arg1 from=symbol_address:@.str3 to=x1`.
- The same prepared summary preserves `@.str0` through
  `slot#11+stack32` and `@.str3` through `slot#12+stack40`; storage plans list
  `@.str0 value_id=1 encoding=frame_slot ... stack_offset=32` and
  `@.str3 value_id=9 encoding=frame_slot ... stack_offset=40`.
- Generated AArch64 materializes `.str0` and `.str3`, then overwrites both
  argument registers from unpublished homes:
  `adrp/add x0, .str0`, `adrp/add x1, .str3`, followed by
  `ldr x0, [sp, #32]`, `ldr x1, [sp, #40]`, `bl fopen`.
- GDB on `build/c_testsuite_aarch64_backend/src/00187.c.bin` stops in
  `_IO_new_file_fopen` for that second `fopen`; the caller is `main` after the
  `bl fopen`, and libc sees `mode=0x0`, matching the unpublished `@.str3`
  stack-home reload.

This is a symbol-address preserved stack-home publication boundary, not an
indexed aggregate selected-address/writeback boundary and not a local buffer
home boundary. The first repair packet should make preserved stack homes for
symbol-address values real before call setup reloads them, or avoid selecting
an unpublished stack home as the source for call arguments.

## Suggested Next

Add focused backend coverage for a direct external call whose string-symbol
arguments are preserved in stack homes across an earlier call, then repair the
shared symbol-address preserved-home publication path before call argument
assignment.

## Watchouts

- Do not fold this residual back into indexed aggregate selected-address or
  selected-snapshot/writeback without fresh generated-code evidence.
- Do not narrow the route to `00187`, `fopen`, one symbol, one stack slot, or
  one emitted call neighborhood. The same generated pattern appears for later
  `fopen` calls and for stack-homed format strings such as `@.str5` and
  `@.str7` before `printf`.
- The next implementation should distinguish between a real populated stack
  home and a planned preserved stack home. The current generated code proves
  the address materialization exists, but it is discarded before the reload.
- Preserve the completed `00130`, `00176`, and `00195` guardrails from idea
  348.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00187_c)$' | tee test_after.log`

Result: build up to date; 4/5 focused tests passed. The remaining failure is
`c_testsuite_aarch64_backend_src_00187_c` with the expected
`RUNTIME_NONZERO` segmentation fault. Proof log: `test_after.log`.

Evidence commands used for localization:

- `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00187.c`
- `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00187.c -o /tmp/c4c_00187_aarch64.s`
- `gdb -q -batch -ex run -ex 'bt' -ex 'info registers x0 x1 x2 x3 sp pc' -ex 'x/12i $pc-24' --args build/c_testsuite_aarch64_backend/src/00187.c.bin`

Recommended narrow proof for the repair packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00187_c)$' | tee test_after.log`
