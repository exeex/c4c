Status: Active
Source Idea Path: ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Helper-Contract Coverage

# Current Packet

## Just Finished

Step 1 audited the prepared RV64 aggregate `va_arg` helper facts for
`tests/c/external/gcc_torture/src/920908-1.c` around function `f`.

Prepared dump boundary:

- BIR has two helpers: `bir.call void llvm.va_arg.aggregate(ptr sret(size=4,
  align=4) %t0, ptr %lv.ap)` at block `entry` inst `1`, and the same helper
  for `%t7` at block `block_2` inst `0`.
- Variadic entry plan for `f`: `named_params=2`, `named_gp=2`,
  `named_fp=0`, `register_save_area required=no`, `overflow_area required=yes
  base_slot=#14 base_stack_offset=56 align=8`, `va_list_layout required=yes
  size=8 align=8 fields=1`, field `overflow_arg_area offset=0 size=8`.
- Helper resources: `scratch_registers=3`, `scratch_stack=0`,
  `helpers=[va_start,va_arg_aggregate]`.
- `va_start` publishes `dst_va_list=%lv.ap:stack_slot:slot=#15:offset=56`
  and `dst_va_list_addr=%lv.ap:register:reg=s1`.

First aggregate helper fact set, block `0` inst `1`:

- Source `va_list`: `src_va_list=%lv.ap:register:reg=s1`; the call-plan ABI
  argument for it is `arg index=1`, `source_encoding=register`,
  `source_reg=s1`, `dest_reg=a1`.
- Destination aggregate/payload: `aggregate_dst=%t0:stack_slot:slot=#10:offset=32`
  and `destination_payload=%t0/stack_slot`; the call-plan aggregate address
  argument is `arg index=0`, `value_bank=aggregate_address`,
  `source_encoding=frame_slot`, `source_value_id=3`, `source_slot=#10`,
  `source_stack_offset=32`, `dest_bank=none`. The address-selection record says
  `selection_source_value=%t0`, `selection_source_home=stack_slot`,
  `selection_source_slot=#3`, `selection_source_stack_offset=20`,
  `selection_source_size=8`, `selection_source_align=8`,
  `selection_materialization_block=entry`,
  `selection_materialization_inst=1`.
- Size/alignment: `payload_size=4`, `payload_align=4`, `copy_size=4`,
  `copy_align=4`, `source_slot=4`, `progression_stride=4`,
  `overflow_stride=4`.
- Helper/access plan: `source_class=overflow_arg_area`,
  `source_field=overflow_arg_area@0`, `source_payload_offset=0`,
  `progression_field=overflow_arg_area@0`,
  `overflow_source_field=overflow_arg_area@0`.
- ABI placement and preservation: callsite wrapper is
  `direct_extern_fixed_arity`; aggregate address is passed as no-register
  aggregate-address operand, and source `va_list` is passed in `a1`. Preserves
  `%ret.sret` in stack slot `#0`, `%lv.ap` in callee-saved `s1`, `%t0` in
  stack slot `#10`, and `%t7` in callee-saved `s2`.
- Frame/stack resources: function summary reports `frame_size=64`,
  `frame_alignment=8`, saved `s1`/`s2`; stack layout has payload local object
  `%t0.0` at slot `#3` offset `20` size `4` align `4`, address/value object
  `%t0` at slot `#10` offset `32` size `8` align `8`, overflow-area base
  object at slot `#14` offset `56`, and `va_start` destination object at slot
  `#15` offset `56` size `8` align `8`.
- Clobbers: helper call records one reserved GPR scratch `t0`, one reserved
  FPR scratch `ft0`, and vector scratch registers `v0` through `v15`.
- Helper result ownership: no scalar result and no returned value; ownership is
  by aggregate destination side effect. The prepared facts publish the
  destination payload home and call argument/address selection rather than a
  result home.

Second aggregate helper fact set, block `2` inst `0`, is the same overflow
shape except the destination is register-backed: `aggregate_dst=%t7:register:reg=s2`,
`destination_payload=%t7/register`, call-plan aggregate address uses
`arg.source_selection=prior_preservation` from `%t7` in callee-saved `s2`, and
addressing records still materialize `%t7` as a frame-slot address at offset
`24`.

Comparison to RV64 object-route needs:

- The first supportable source shape is explicit overflow-area aggregate copy:
  load current overflow pointer from `%lv.ap` field offset `0`, copy `4` bytes
  at source payload offset `0` into the prepared destination payload, then
  advance the overflow-area field by stride `4`.
- The available facts are sufficient for source `va_list` placement, va-list
  field layout, overflow base state, copy size/alignment, progression stride,
  helper resources, ABI argument placement, frame resources, and clobber policy.
- The missing/narrow contract item for direct RV64 lowering is not the
  overflow access plan itself; it is a single prepared helper-contract fact
  tying `destination_payload=%t0/stack_slot` to the concrete payload write
  address/object (`%t0.0`, slot `#3`, offset `20`, size `4`, align `4`) without
  reconstructing that relationship from the call argument's address-selection
  side channel or from raw BIR/addressing spelling. Until that fact is part of
  the aggregate helper contract, the first shape should stay fail-closed rather
  than lower directly.

## Suggested Next

Execute Step 2 by adding focused helper-contract coverage for the RV64
overflow-area aggregate `va_arg` shape, including a positive prepared dump
expectation for the facts above and a fail-closed case for a missing concrete
destination payload address/home fact.

## Watchouts

- Do not reopen shared LIR/BIR vararg semantics from idea 360.
- Do not reopen `va_start` destination-address or overflow-area setup from
  ideas 365 and 366.
- Do not infer aggregate layout, helper resources, or `va_list` state from
  source syntax, testcase names, raw BIR text, or diagnostics.
- Preserve fail-closed diagnostics for unsupported aggregate helper shapes.
- The first helper looks nearly lowerable, but direct RV64 object emission
  should not derive the destination payload slot by joining `%t0`,
  `%t0.0`, call argument source selection, and addressing records ad hoc.

## Proof

Audit-only packet; no build/test proof was required, and no canonical proof
logs were created or overwritten.

Read-only diagnostics run:

- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/920908-1.c`
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/920908-1.c -o /tmp/920908-1.o`
  confirmed the current boundary:
  `unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_arg_aggregate helper`.
- A temporary prepared dump was written to `/tmp/920908-1.prepared` for
  read-only slicing with `rg`.
