Status: Active
Source Idea Path: ideas/open/638_rv64_string_label_pointer_runtime_object_correctness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify the first owner

# Current Packet

## Just Finished

Step 2 classified the first implementation owner for `src/20000722-1.c` as
call lowering: RV64 call-argument lowering mishandles a local frame-slot address
selected from `local_frame_address_materialization`.

Supporting evidence is recorded in
`build/agent_state/638_step2_owner_classification.txt`. Prepared BIR has the
right semantic source for the call, `bir.call void foo(ptr %lv._clit_)`, and
records `call_arg_source index=0 encoding=frame_slot source_value=%lv._clit_
selection=frame_slot_address`. The prepared call plan also records
`arg.source_selection=local_frame_address_materialization` with
`selection_source_slot=#0` and `selection_source_stack_offset=0`, but then homes
`%lv._clit_` in `s2` and lowers the call move as
`reason=call_arg_register_to_register` into ABI register `a0`.

The linked c4c binary follows that bad source choice: `bar` stores the compound
literal at `0(sp)` and `8(sp)`, materializes `.str0` at `7dc`, then calls `foo`
with `mv a0,s2`. The clang binary stores the same literal in its frame and
passes the frame-slot address with `addi a0,s0,-32`.

Rejected owners:
- string-label address materialization: rejected; prepared BIR has
  `layout_authority=string_constant_label_pointer` and string-constant address
  materialization, the c4c object has `.str0` PC-relative relocations, and the
  linked binary materializes `.str0` before storing it into the local object.
- local/global memory: rejected; c4c emits the local compound-literal stores at
  the expected pointer and `int t` offsets before the call, and no global object
  lane is involved except the already-materialized string literal.
- stack layout: rejected; prepared slots #0..#5 map to offsets 0, 8, 12..15,
  and the linked c4c frame keeps local data and saved registers in distinct
  locations.
- ABI/call setup as a convention failure: rejected; `a0` is the correct first
  pointer argument register, but c4c fills it from the wrong source.
- branch/control flow: rejected as first owner; `foo` receives a bad pointer
  before its `lw 8(a0)` and branch sequence can be judged.
- true runtime support: rejected; clang qemu exits 0, c4c qemu exits 139 with
  empty output, and the object/link path is live.

## Suggested Next

Step 3 should recommend a focused implementation idea for RV64 call-argument
lowering of address-materialized local frame-slot values. The proof surface
should include the existing one-row RV64 GCC torture backend object run for
`src/20000722-1.c` and a narrow backend assertion that
`arg.source_selection=local_frame_address_materialization` lowers to a selected
frame-slot address calculation rather than a register-home copy.

## Watchouts

- This is a research route; do not make implementation edits inside this idea.
- The bad fact is not that the ABI destination register is wrong; `a0` is
  correct. The bad fact is that the call-argument source is copied from stale
  register home `s2` instead of materializing the selected local frame-slot
  address.
- Do not claim progress from expectation, unsupported-marker, allowlist,
  timeout/accounting, runtime-comparison, or pass/fail accounting changes.
- Keep the follow-up single-owner. Do not widen it into generic runtime support,
  string-literal policy, stack layout, or branch/control-flow work unless fresh
  evidence appears after the call-lowering issue is fixed.

## Proof

No proof refresh was run for this research-only packet. Classification used the
saved Step 1 artifacts under `build/agent_state/638_step1_runtime_object/`,
especially `step2_owner_evidence_extract.txt`, `prepared_bir.txt`,
`c4c_o_objdump_dr.txt`, `c4c_bin_objdump_dr.txt`,
`clang_bin_objdump_dr.txt`, readelf outputs, and qemu rc/stdout/stderr files.
Existing proof log remains `build/agent_state/638_step1_runtime_object/test_after.log`.
