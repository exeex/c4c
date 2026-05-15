Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Variadic Consumption Boundary

# Current Packet

## Just Finished

Step 1: Inspect Prepared Variadic Consumption Boundary completed as an
inspection-only packet.

Current helper record path:

- `va_start`: HIR builtin emits `LirVaStartOp{ap_ptr}`; BIR lowers it to
  `CallInst{callee="llvm.va_start.p0", args=[ap_ptr], arg_types=[Ptr],
  return_type=Void}`; AArch64 dispatch classifies it as
  `PreparedVariadicEntryHelperKind::VaStart`, requires a
  `PreparedVariadicEntryPlanFunction`, records `source_variadic_entry` and
  `variadic_entry_helper` in `CallInstructionRecord`, then
  `make_call_instruction` keeps the node `deferred_unsupported` with
  diagnostic `variadic entry helper machine-node lowering requires a delegated
  consumption slice`.
- Scalar `va_arg`: LIR `LirVaArgOp{result, ap_ptr, type_str}` with a scalar
  result lowers to `CallInst{result=<typed scalar>, callee="llvm.va_arg.<type>",
  args=[ap_ptr], arg_types=[Ptr], return_type=<scalar>}`; AArch64 dispatch
  classifies any non-aggregate `llvm.va_arg.*` helper as
  `PreparedVariadicEntryHelperKind::VaArg` and follows the same deferred call
  record path.
- Aggregate `va_arg`: LIR aggregate `LirVaArgOp` declares local aggregate
  result storage, aliases the result to that storage, and lowers to
  `CallInst{callee="llvm.va_arg.aggregate", args=[result_ptr, ap_ptr],
  arg_types=[Ptr, Ptr], return_type=Void}`; AArch64 dispatch classifies it as
  `PreparedVariadicEntryHelperKind::VaArgAggregate` and follows the same
  deferred call record path.
- `va_copy`: HIR builtin emits `LirVaCopyOp{dst_ptr, src_ptr}`; BIR lowers it
  to `CallInst{callee="llvm.va_copy.p0.p0", args=[dst_ptr, src_ptr],
  arg_types=[Ptr, Ptr], return_type=Void}`; AArch64 dispatch classifies it as
  `PreparedVariadicEntryHelperKind::VaCopy` and follows the same deferred call
  record path.

Exact operands each selected helper would need:

- `va_start`: destination `va_list` home from `ap_ptr`; field layout for
  `overflow_arg_area`, `gp_register_save_area`, `fp_register_save_area`,
  `gp_offset`, and `fp_offset`; register-save-area frame slot and stack
  offset; GP/FP save-area byte offsets and slot sizes; overflow-area base slot
  and stack offset; named GP/FP register counts or the derived initial
  GP/FP offsets; scratch registers/stack needed to materialize field stores.
- Scalar `va_arg`: source `va_list` home from `ap_ptr`; typed scalar result
  home from `CallInst::result`; field layout and writable offset fields;
  register-save-area slot/offset plus GP/FP offsets, slot sizes, saved counts,
  and current/progressed offset values; overflow-area base slot/offset and
  alignment; result width/class/alignment; scratch registers/stack for address
  calculation, load, conditional register-save-vs-overflow selection, and
  `va_list` progression.
- Aggregate `va_arg`: destination aggregate payload storage from
  `result_ptr`; source `va_list` home from `ap_ptr`; aggregate size/alignment
  from the BIR argument ABI/layout; field layout and writable progression
  fields; register-save-area and overflow-area slot/offset facts for payload
  source selection; copy source/destination operands; scratch registers/stack
  for address calculation and aggregate copy.
- `va_copy`: destination `va_list` home from `dst_ptr`; source `va_list` home
  from `src_ptr`; complete `va_list_layout` size/alignment and fields; copy
  source/destination memory operands for the whole record or per-field copies;
  scratch resources for the copy sequence.

Comparison to idea-232 prepared facts:

- Present: helper classification for `va_start`, scalar `va_arg`, aggregate
  `va_arg`, and `va_copy`; variadic callee `PreparedVariadicEntryPlanFunction`
  lookup; named GP/FP register counts when parameter ABI is available; AAPCS64
  register-save-area size/alignment, GP/FP byte offsets, GP/FP slot sizes,
  saved GP/FP counts, and initial GP/FP offsets; overflow-area required flag
  and alignment; `va_list` size/alignment and five AAPCS64 field descriptors;
  fail-closed dispatch and printer diagnostics.
- Missing by explicit prepared metadata: `register_save_area.slot_id`,
  `register_save_area.stack_offset_bytes`, and
  `overflow_area.base_stack_offset_bytes` are added to
  `missing_required_facts` by the prepared population path.
- Missing by absent prepared fields: `overflow_area.base_slot_id` exists in
  the struct and is required by AArch64 completeness checks, but the AAPCS64
  population path does not fill it or add it to `missing_required_facts`;
  `helper_resources.scratch_register_count` and
  `helper_resources.scratch_stack_bytes` exist and are required whenever any
  helper is present, but are not populated by idea 232.
- Missing for selected-node operands: there is no per-helper prepared effect
  record that maps `ap_ptr`, scalar result, aggregate result pointer, or
  `va_copy` source/destination pointers to concrete `MemoryOperand` homes;
  current `CallInstructionRecord` preserves only the BIR call operands and
  `source_variadic_entry`, not helper-specific selected-node operands.

Initial focused proof subset to use once the missing facts are supplied:

`ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_aarch64_machine_printer)$'`

Decision: Step 2 should not proceed directly to `va_start` consumption. Split
out a prepared/shared authority initiative for variadic entry frame storage
and helper resources first: fill `register_save_area.slot_id`,
`register_save_area.stack_offset_bytes`, `overflow_area.base_slot_id`,
`overflow_area.base_stack_offset_bytes`, helper scratch counts/stack bytes, and
helper operand-home facts for `va_list` pointers and helper destinations.

## Suggested Next

Supervisor should route to plan-owner to split a narrower prepared/shared
initiative for AArch64 variadic entry storage and helper operand-home authority
before any Step 2 `va_start` machine-node consumption packet.

## Watchouts

- Do not infer AAPCS64 variadic layout, register-save offsets, overflow-area
  offsets, named argument counts, or scratch policy inside AArch64 codegen.
- Do not weaken existing fail-closed diagnostics or backend expectations to
  claim support.
- Treat prepared dump presence as necessary context, not proof of selected
  machine-node consumption.
- `overflow_area.base_slot_id` is checked by dispatch completeness but is not
  currently added to `missing_required_facts` by AAPCS64 prepared population;
  keep that distinction visible in the split initiative.

## Proof

Inspection-only packet. No build or test proof was required by the delegated
packet, and no `test_after.log` update was made.
