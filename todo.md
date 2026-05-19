Status: Active
Source Idea Path: ideas/open/320_aarch64_f128_transport_addressability.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize F128 Transport Address Shape

# Current Packet

## Just Finished

Step 1 localized the `00204.c` AArch64 failure to the `f128_transport` machine
printer path. The focused proof still fails at machine function 0, block 0,
machine instruction 758:

`family=f128_transport opcode=f128_transport: f128 memory transport address is
not printable`.

The generated AArch64 prepared dump for `stdarg` shows the implicated transport
family as 16-byte `f128` local-memory copies through prepared F128 carriers.
The representative shape at the current instruction neighborhood is:

- BIR: `%t262.aggregate.copy.16 = bir.load_local f128 %t261.16`
- prepared memory access: `access block=entry inst_index=758 base=frame_slot
  result=%t262.aggregate.copy.16 frame_slot=#1986 offset=0 size=16 align=16
  base_plus_offset=yes`
- source frame slot: `slot #1986 ... offset=2832 size=16 align=16`
- destination carrier: `f128_carrier %t262.aggregate.copy.16 value_id=2712
  kind=memory_backed size=16 align=16 bank=fpr class=float width=1
  slot_id=#3022 stack_offset=13600`

The owning code surface is `src/backend/mir/aarch64/codegen/f128.cpp`,
especially `print_f128_transport()` and its helper
`f128_memory_backed_carrier_address()`. The construction path is
`src/backend/mir/aarch64/codegen/memory.cpp::lower_f128_transport_instruction()`
through `make_prepared_f128_carrier_transport_record()`, but the selected record
is present; the failure is at final address spelling/materialization. The
printer currently calls the plain `memory_address()` helper for the transport
memory operand, while adjacent machine-printer paths use
`printable_memory_address()` / frame-slot scratch materialization for large or
non-direct frame-slot addresses.

## Suggested Next

Execute Step 2 of `plan.md`: repair `print_f128_transport()` so 16-byte F128
memory transports can print or materialize prepared frame-slot addresses through
the same general scratch-address path used by adjacent AArch64 memory printers.
Add focused `backend_aarch64_machine_printer` coverage for a memory-backed F128
carrier load/store whose transport memory is a nontrivial frame slot.

## Watchouts

- Preserve the fixed HFA lane repair, caller-side variadic HFA lane handoff,
  and callee-side aggregate `va_arg` helper handoff from idea 319.
- Do not reopen scalar ALU immediate materialization, raw `va_start` helper
  text, frame adjustment materialization, stack-offset spelling, runner
  behavior, timeout behavior, proof-log policy, or c-testsuite expectations.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  output line, one register, one stack slot, or one instruction index.
- The separate global data emission symptom still exists in generated assembly.
  Do not mix global initializer emission into this packet unless it becomes the
  first bad fact after `f128_transport` addressability advances.
- The AArch64 prepared dump also contains global-symbol F128 aggregate loads
  such as `base=global_symbol symbol=hfa34 offset=... size=16 align=16`.
  Do not fold global initializer emission into this repair, but keep symbol
  memory operands in mind if the first bad fact advances from frame-slot
  transport to symbol-address transport.
- Avoid a named `00204.c` or `%t262` fix. The repair should be a general
  `F128TransportRecord` addressability/materialization rule.

## Proof

Ran the exact delegated proof command and wrote output to `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Build succeeded. The focused CTest subset ran 11 tests: 10 passed, and the only
failure was the expected baseline `c_testsuite_aarch64_backend_src_00204_c`
`f128_transport address is not printable` failure.
