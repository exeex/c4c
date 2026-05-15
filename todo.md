Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect I128 Prepared And AArch64 Surfaces

# Current Packet

## Just Finished

Step 1 inspection completed for prepared i128 carriers, AArch64 selected
record/dispatch/printer surfaces, and runtime helper-call boundaries.

Current prepared/shared facts:

- `I128` is not register-allocated by default: `classify_register_class`
  returns `PreparedRegisterClass::None`, while generic register-group
  overrides can force a class plus `contiguous_width`.
- Prepared storage/value homes have a single value home plus generic grouped
  metadata: `PreparedStoragePlanValue::contiguous_width`,
  `occupied_register_names`, optional register placement, and stack-slot
  encoding. Those fields prove occupancy, but do not name semantic low/high
  lanes or lane byte order.
- Call-boundary facts carry grouped register width/occupancy and placements for
  arguments, results, preserved values, and clobbers. I128 ABI inference
  currently treats normal i128 arguments/results as memory-backed/sret-style
  storage rather than an AAPCS64 low/high GPR pair.
- Memory-backed facts exist as generic stack slots and `PreparedMemoryReturnPlan`
  size/alignment/storage, but there is no i128-specific carrier that states
  low-lane/high-lane offsets or whether a value is pair-backed versus
  memory-backed for target lowering.

Current AArch64 gaps:

- Scalar register conversion rejects `I128`: `scalar_register_view`,
  `scalar_fp_register_view`, and `scalar_storage_register_view` return no view
  for `I128`, so prepared scalar ALU/cast records fail closed.
- Existing `ScalarInstructionRecord`, `MemoryInstructionRecord`,
  `ReturnInstructionRecord`, and `CallInstructionRecord` have no i128 pair
  operand record with structured low/high source/result lanes.
- Return lowering carries at most one operand and diagnoses unsupported typed
  operand authority for values it cannot resolve; it has no pair-return record.
- Machine printer paths know scalar integer/FP, memory, calls, and returns, but
  no i128 pair transport/arithmetic/shift/compare/helper sequence. Legacy
  i128 notes under `codegen/i128_ops.md` are deferred reference material only.
- Runtime helper boundaries have generic call plans, clobbers, preserved-value
  facts, and direct-call printing, but no structured i128 helper-call record
  that names helper kind, argument pair lanes, result lanes, scratch/clobber
  requirements, or ABI transition facts.

Blocker for AArch64 selection:

Prepared currently does not expose authoritative i128 low/high pair or
memory-backed lane carriers. AArch64 would have to infer low/high order from
rendered register names, contiguous numeric adjacency, fixed `x0`/`x1`
conventions, or generic stack offsets. That is explicitly out of scope for this
route.

## Suggested Next

Execute Step 2 as a prepared/shared authority packet before AArch64 selection:
add an explicit i128 carrier beside existing value-home/storage facts that can
represent either `RegisterPair` or `MemoryBacked` authority.

Required carrier fields:

- prepared value id/name, function identity, source BIR type, and carrier kind
- low/high lane order, each lane width, and total size/alignment
- for register-pair values: low/high register placements or names, register
  bank/class, occupied-register provenance, and result/source authority
- for memory-backed values: slot id, stack/base offset, low/high byte offsets,
  size/alignment, and ownership/provenance
- call/helper boundary facts for i128 arguments/results/clobbers only when the
  ABI policy is explicit; otherwise diagnose missing pair ABI policy instead
  of defaulting to fixed registers

Focused proof command for the Step 2 packet:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepare_stack_layout|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch)$') > test_after.log 2>&1`

## Watchouts

- Do not create a local i128 allocator in AArch64 codegen.
- Do not infer low/high pair homes from rendered register names or fixed
  `x0`/`x1` accumulator conventions.
- Do not lower i128 as scalar i64 or claim progress through named testcase
  shortcuts.
- If prepared low/high or memory-backed carrier authority is missing, stop and
  split the exact prepared/shared blocker instead of filling it in target
  lowering.
- Do not treat generic `contiguous_width == 2` or
  `occupied_register_names.size() == 2` as semantic i128 low/high authority.
- Do not resurrect legacy helper conventions from `codegen/i128_ops.md`; helper
  calls need structured argument/result/clobber records first.
- Keep binary128/F128 and float/i128 conversions separate until helper-call
  facts carry explicit register-bank transition authority.

## Proof

Inspection-only packet. No build/tests were run, and `test_after.log` was not
created or modified.
