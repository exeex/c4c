Status: Active
Source Idea Path: ideas/open/324_aarch64_variadic_frame_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Frame And Formal Publication Fault

# Current Packet

## Just Finished

Step 1 localized the AArch64 variadic frame/formal publication fault for idea
324.

Fresh generated artifact: `build/c_testsuite_aarch64_backend/src/00204.c.s`.
`myprintf` allocates `896` bytes:

`sub sp, sp, #896`

but the first loop path calls `match` with an out-of-frame second argument:

`ldr x1, [sp, #9696]`

The prepared dump for `myprintf` reports ordinary frame state near the emitted
prologue size:

`prepared.summary @myprintf stable_base=rsp frame_size=848 ... saved_regs=4`

and then publishes call-preservation stack homes far outside that frame, for
example:

`%p.format#2065:stack_slot:slot#2729:stack+9680:size=8:align=8`
`%t5#2074:stack_slot:slot#2731:stack+9696:size=8:align=8`

The bad frame path is therefore the regalloc/call-preservation stack-slot
publication path, not the AArch64 memory printer. `build_call_preserved_values`
in `src/backend/prealloc/call_plans.cpp` trusts `PreparedValueHome` /
`PreparedRegallocValue::assigned_stack_slot`, while
`populate_frame_plan()` in `src/backend/prealloc/frame_plan.cpp` sizes a
function from `prepared.stack_layout.frame_slots` plus saved registers. The
regalloc-assigned call-crossing spill homes visible to call plans are not
published into the same per-function frame accounting before AArch64 printing
emits them as `[sp, #...]`.

The fixed-formal fault is separate but adjacent. The `llvm.va_start.p0`
callsite is prepared as:

`arg0 bank=gpr from=register:x21 to=x0`

and the generated entry emits:

`mov x0, x21`
`add x21, sp, #816`
...
`str x0, [sp]`

That clobbers the incoming fixed formal `%p.format` in `x0` before the
semantic `bir.store_local %lv.s, ptr %p.format` is materialized, so the format
loop starts from the `va_list` helper argument instead of the caller-provided
format pointer. The owner is fixed-formal publication/preservation across the
entry `va_start` helper move: either formal parameters must get valid homes
before helper argument movement, or the helper lowering must not destroy live
fixed-formal argument registers before their stores.

Responsibility split:

- Frame-size accounting: `src/backend/prealloc/frame_plan.cpp::populate_frame_plan()`
  must size the active function for every emitted stack slot, including
  regalloc/call-preservation homes, before AArch64 prologue lowering.
- Local stack-slot publication: `src/backend/prealloc/regalloc.cpp` and
  `src/backend/prealloc/regalloc/stack_slots.cpp` create assigned stack slots;
  those slots need function-local publication rather than module-global offsets
  that later appear as `stack+9680`.
- Spill-slot materialization: `src/backend/prealloc/call_plans.cpp::build_call_preserved_values()`
  and AArch64 `src/backend/mir/aarch64/codegen/calls.cpp` consume the published
  homes correctly enough to expose the bad offsets; the printer is not the
  first owner.
- Fixed-formal preservation: entry lowering for variadic helpers must preserve
  incoming fixed formals such as `%p.format` before `llvm.va_start.p0` argument
  moves reuse `x0`.

## Suggested Next

Run Step 2: repair the prepared publication side so regalloc/call-preservation
stack slots are function-local frame slots included in
`PreparedFramePlanFunction::frame_size_bytes`, and preserve fixed formals before
the entry `va_start` helper argument move can overwrite their incoming ABI
registers. Add focused backend checks around a variadic function that both
spills call-crossing values and consumes its fixed format parameter after
`va_start`.

## Watchouts

- Preserve prior repairs: large stack offsets, large frame adjustments,
  `va_start` helper lowering, scalar ALU immediates, HFA argument lanes, F128
  transport, aggregate helper text lowering, `va_start` destination
  materialization, and aggregate/floating `va_arg` source/progression.
- Raw `va.arg.aggregate*` text must stay absent.
- HFA/floating aggregate consumers should keep FP register-save-area source
  selection with `FpOffset` progression and overflow fallback unless generated
  evidence proves that path still owns the first bad fact.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, `format`, `x0`, `x21`,
  one local, one stack slot, one offset, or one emitted instruction sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Raw `va.arg.aggregate*` text is still absent; the current first bad facts are
  frame/formal publication and entry fixed-formal preservation.
- The smallest representative local surfaces are prepared-BIR/function summary
  dumps for `00204.c`, `backend_aarch64_machine_printer`,
  `backend_aarch64_target_instruction_records`, and
  `backend_aarch64_instruction_dispatch`.

## Proof

Reran the delegated focused proof into `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Build succeeded. CTest ran 11 tests: 10 passed, 1 failed. The only failure is
`c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`.

The focused proof scope is sufficient for the Step 2 repair loop because it
includes the representative external case, prepared-BIR handoff/focus dumps,
and the AArch64 target instruction, machine printer, and dispatch guardrails.
Proof log path: `test_after.log`.
