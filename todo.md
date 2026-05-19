Status: Active
Source Idea Path: ideas/open/327_aarch64_fixed_formal_entry_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair General Fixed Formal Entry Publication

# Current Packet

## Just Finished

Step 2 repaired the remaining callee-side byval/frame-slot aggregate formal
publication gap for small AArch64 integer aggregate formals, but the delegated
proof is not fully green.

- `lower_entry_formal_publications` now includes byval pointer formals whose
  ABI metadata says the aggregate copy arrives in integer registers. It
  expands the incoming `xN`/`xN+1` lanes into exact `strb` writes to the
  prepared stack home instead of treating the byval pointer as an 8-byte scalar
  pointer.
- The store-side publication path now recognizes loads from a byval formal's
  prepared stack home as loads from the aggregate object itself, then publishes
  those bytes into the store source register before local aggregate-copy
  stores.
- Added focused `backend_aarch64_instruction_dispatch` coverage for a 9-byte
  byval fixed formal arriving in `x0`/`x1` and prepared into a stack home. The
  test proves exact byte publication into the frame slot before the inline
  `va_start` helper.
- Generated `00204.c` AArch64 now shows `fa_s1`/`fa_s2` publishing incoming
  byval bytes before first use. For example, `fa_s1` starts with `strb w0,
  [sp]` and then reloads `ldrb w13, [sp]` before copying to local slots;
  `fa_s2` publishes byte 0 from `w0` and byte 1 via `lsr x9, x0, #8` before
  the copy stores.

## Suggested Next

Smallest next packet: repair or split out AArch64 byval aggregate call-argument
publication from prepared frame slots into ABI register lanes. The new first
bad fact is on the caller side in `arg`, not the callee fixed-formal entry
path: before `bl fa_s1`, generated assembly copies `s1` into a prepared stack
slot at `[sp, #928]` but never moves that byte into `x0`; before `bl fa_s2`,
the callsite likewise prepares local stack bytes and then copies an unrelated
stack word to `[sp]` instead of publishing the two-byte aggregate into `x0`.
The callee now expects the ABI register lanes and publishes them correctly on
entry, so the remaining `00204.c` segfault is a callsite byval ABI handoff
gap.

## Watchouts

- Do not treat the remaining runtime failure as evidence that callee-side
  byval/frame-slot fixed-formal publication failed; the representative
  `fa_s1`/`fa_s2` assembly now publishes and reloads the incoming frame-slot
  bytes before the aggregate copy stores.
- The next gap is broader than fixed-formal entry publication: byval aggregate
  call arguments need publication from prepared stack/object bytes into the
  ABI-defined argument register lanes for sizes 1 through 16, while larger
  byval aggregates such as `s17` remain stack-passed.
- Avoid papering over `00204.c` by changing expectations; the failing early
  runtime still comes from a real AArch64 aggregate ABI handoff gap.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build passed; 11/12 selected tests passed. The only failure remains
`c_testsuite_aarch64_backend_src_00204_c`, `[RUNTIME_NONZERO]` with
`exit=Segmentation fault`. The previous byval/frame-slot fixed-formal
publication blocker is repaired; the remaining failure is now attributable to
the separate byval aggregate callsite register-lane publication blocker
described above.

Proof log path: `test_after.log`.
