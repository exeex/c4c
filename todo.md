Status: Active
Source Idea Path: ideas/open/327_aarch64_fixed_formal_entry_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Fixed Formal Entry Publication Gap

# Current Packet

## Just Finished

Step 1 localized the fixed-formal entry publication gap for idea 327.

- Fixed formal: `myprintf` parameter `%p.format` from
  `void myprintf(const char *format, ...)`.
- Semantic BIR: `bir.func @myprintf(ptr %p.format) -> void`, with entry block
  `llvm.va_start.p0(ptr %lv.ap)` followed by
  `bir.store_local %lv.s, ptr %p.format`.
- Prepared storage: `%p.format` has `value_id=2043` and
  `storage %p.format ... encoding=register bank=gpr ... reg=x13`;
  `%lv.s` has `value_id=2051` and a callee-saved register home `x20`, while
  the address-exposed local slot for `s` is frame object `#1653`.
- ABI incoming location: first fixed pointer formal arrives in AAPCS64 `x0`.
- Assigned prepared home: prepared/regalloc assigns the formal to caller-saved
  `register:x13`, not to `x0` or a stack home.
- First generated consumer: generated `myprintf` prologue stores `str x13,
  [sp]` for the entry `store_local %lv.s, ptr %p.format`, then
  `ldr x13, [sp]; mov x10, x13; ldrb w9, [x10]`; no earlier `mov x13, x0`,
  `mov x20, x0`, or stack store from `x0` exists.
- Likely owning code surfaces:
  `src/backend/mir/aarch64/codegen/dispatch.cpp:3991`
  `lower_entry_formal_publications`, which is inserted before block-entry
  moves and the `va_start` helper at `dispatch.cpp:4162`, but currently only
  publishes fixed formals whose prepared home is `StackSlot`; register homes
  like `%p.format -> x13` are skipped.
- Supporting consumer surface:
  `src/backend/mir/aarch64/codegen/dispatch.cpp:2738`
  `lower_store_local_with_address_materialization` routes the entry
  `store_local`, and `src/backend/mir/aarch64/codegen/memory.cpp:1526`
  builds the prepared store using the storage plan, so it faithfully consumes
  the unpublished `x13` home.
- Existing focused stack-home coverage:
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp:8017`
  proves fixed formals assigned to stack homes are published before
  `va_start`, but it does not cover register homes.

## Suggested Next

Execute Step 2 as the smallest repair/proof packet: extend the general
AArch64 fixed-formal entry publication path so a fixed formal with an ABI
incoming register and a different prepared register home emits a register move
before `va_start` and before first use; add/extend a focused instruction
dispatch test for a variadic callee whose fixed pointer formal is assigned to a
non-ABI register home.

## Watchouts

- Keep the repair general over fixed formals and prepared homes; do not
  special-case `00204.c`, `myprintf`, `%p.format`, `x0`, `x13`, one pointer
  type, or one emitted instruction sequence.
- Preserve the existing stack-home publication behavior and ordering before
  the inline variadic `va_start` helper.
- `x13` is also the call-result/clobber scratch register in adjacent paths, so
  the repair should publish at function entry before calls and should avoid
  relying on later local-store lowering to synthesize the bridge.
- Focused representative tests for the next packet: add register-home coverage
  under `backend_aarch64_instruction_dispatch`, then run the delegated subset
  including `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
  and `c_testsuite_aarch64_backend_src_00204_c`.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build passed; 11/12 selected tests passed. The only failure was the
representative `c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO]` with `exit=Segmentation fault`, consistent with this
localization-only packet and the missing fixed-formal entry publication.

Proof log path: `test_after.log`.
