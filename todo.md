Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Byval Aggregate Call-Argument Lane Gap

# Current Packet

## Just Finished

Step 1 - Localize Byval Aggregate Call-Argument Lane Gap:
caller-side AArch64 byval aggregate publication is localized in `arg` for the
first two `00204.c` byval callsites.

Semantic BIR keeps the source intent visible:

- `fa_s1`: `%t2.global.aggregate.load.0 = bir.load_local i8 %t2.0, addr s1`;
  `bir.store_local %t2.0, i8 %t2.global.aggregate.load.0`; then
  `bir.call void fa_s1(ptr byval(size=1, align=1) %t2)`.
- `fa_s2`: `%t3.global.aggregate.load.0` and
  `%t3.global.aggregate.load.1` load `s2` and `s2+1`, store to `%t3.0` and
  `%t3.1`, then `bir.call void fa_s2(ptr byval(size=2, align=1) %t3)`.

Prepared/call-plan state chooses stack delivery instead of the required
register-lane delivery:

- `fa_s1` callsite `block=0 inst=3`: `arg0 bank=aggregate_address
  from=register:x21 to=stack+0`; `%t2` is also preserved in `x21`.
- `fa_s2` callsite `block=0 inst=8`: `arg0 bank=aggregate_address
  from=frame_slot:stack+1224 to=stack+0`; `%t3` is preserved in
  `slot#2723+stack1224`.

The prepared source bytes are present before the calls:

- `s1[0]` is copied into caller frame byte `[sp, #928]` before `bl fa_s1`.
- `s2[0]` and `s2[1]` are copied into caller frame bytes `[sp, #929]` and
  `[sp, #930]` before `bl fa_s2`; the prepared preserved aggregate slot is
  `stack+1224`.

AAPCS64 register lanes expected by the callees:

- `fa_s1` consumes byte 0 from `w0`/low `x0`; generated callee entry starts
  with `strb w0, [sp]`.
- `fa_s2` consumes bytes 0 and 1 from low `x0`; generated callee entry uses
  `strb w0, [sp]` and `lsr x9, x0, #8; strb w9, [sp, #1]`.

The emitted caller gap is before `bl`: after preparing the aggregate bytes,
`arg` emits no `ldrb`/pack into `w0` or `x0` before `bl fa_s1`, and no
two-byte pack into low `x0` before `bl fa_s2`.

Likely owning code surfaces:

- `src/backend/bir/lir_to_bir/calling.cpp` currently emits byval call
  argument ABI records as `type=Ptr`, `primary_class=Memory`, `byval_copy=true`
  without publishing AArch64 register-passed small-aggregate lane ABI.
- `src/backend/prealloc/regalloc/call_return_abi.cpp` makes
  `call_arg_storage_kind()` return `StackSlot` for any `byval_copy` on AArch64,
  so `append_call_arg_move_resolution()` in
  `src/backend/prealloc/regalloc/call_moves.cpp` records stack delivery rather
  than call-argument register delivery.
- `src/backend/prealloc/call_plans.cpp` faithfully prints those stack
  destinations into prepared call plans.
- `src/backend/mir/aarch64/codegen/calls.cpp` has existing scalar
  frame-slot-to-register and aggregate stack-copy lowering, but no current
  before-call path that loads/combines register-passed byval aggregate bytes
  from prepared storage into ABI integer lanes.

## Suggested Next

Smallest next repair packet: teach the AArch64 byval call-argument ABI path to
classify small byval aggregates that fit AAPCS64 integer registers as register
destinations, then lower the before-call publication from prepared aggregate
storage into low-to-high `xN` lane bytes before `bl`. Prove first with a
focused local backend test that covers size 1 and size 2 aggregate register
publication from prepared storage, then include the delegated 00204/guardrail
subset.

## Watchouts

Do not reopen fixed-formal entry publication: generated `fa_s1` and `fa_s2`
already consume `x0` lanes on entry. Do not make all byval aggregates
register-passed: `s17` and larger stack-passed shapes must stay stack-passed.
The repair should avoid named `00204.c`/`fa_s1`/`fa_s2` handling and should
generalize across small byval aggregate sizes 1 through 16 where AAPCS64 uses
integer registers.

Focused tests to keep in the repair proof:
`backend_lir_to_bir_notes`, `backend_cli_dump_bir_00204_stdarg_semantic_handoff`,
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff`,
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
`backend_prepare_liveness`, `backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_target_instruction_records`, and
`c_testsuite_aarch64_backend_src_00204_c`.

## Proof

Delegated proof written to `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build completed with `ninja: no work to do`; CTest ran 12 tests, 11
passed and 1 failed. The failing test was
`c_testsuite_aarch64_backend_src_00204_c`, which exits with
`[RUNTIME_NONZERO] .../00204.c exit=Segmentation fault`. This is the
unrepaired runtime symptom localized above. All focused BIR/prepared-BIR and
AArch64 backend guardrail tests in the delegated subset passed.
