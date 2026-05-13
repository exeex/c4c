Status: Active
Source Idea Path: ideas/open/213_aarch64_allocation_to_target_mir_move_spill_reload_records.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Existing Record Coverage

# Current Packet

## Just Finished

Step 1 inspected existing AArch64 target-MIR allocation record coverage against
`src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`, focused on
`module/module.hpp`, `module/module.cpp`, and markdown consumers.

Current coverage:

- `TargetRegisterRecord` exists and is populated from prepared value homes,
  regalloc assignments, spill-register authority, and storage plans. It keeps
  value id/name, prepared register class/bank where available, allocation pool
  derived from parsed AArch64 register spelling, physical-register display
  spelling, contiguous width, occupied-register spellings, long-lived-home
  eligibility, and a snapshot/reference kind. Gaps: no final typed AArch64
  register reference is stored in the record; value-home records currently use
  `PreparedRegisterClass::None`/`PreparedRegisterBank::None`; there is no
  explicit future virtual-register placeholder or placeholder owner; scratch
  exclusion is inferred through kind/pool/long-lived flags rather than a direct
  reserved-scratch policy field.
- `OperandRecord` exists and merges prepared value homes, regalloc values, and
  storage plans. It keeps prepared value id/name, function, type, value kind,
  stack object id, home kind, storage encoding, register class, storage bank,
  group width, frame-slot id, prepared stack-offset snapshot, immediates,
  symbols, pointer-base identity/delta, source prepared pointers, and indexes
  into `TargetRegisterRecord` for value-home, assigned, spill-authority, and
  storage registers. Gaps: spill-slot authority is only frame-slot id plus
  offset snapshot, without explicit spill-slot size/alignment/lifetime or a
  distinct allocation spill-slot id; no non-register location variant/authority
  kind beyond existing storage/home enums; no future virtual-register
  placeholder representation.
- `MoveRecord` exists and is populated from value-location move bundles and
  regalloc move resolutions. It keeps phase, authority kind,
  source/destination value ids, destination kind/storage kind, ABI index,
  destination register spelling, destination prepared stack-offset snapshot,
  execution site, cycle-temp/coalescing flags, parallel-copy source metadata,
  immediate source, op kind, reason, and source prepared pointers. Gaps:
  prepared `destination_contiguous_width` and
  `destination_occupied_register_names` are not copied; destination register is
  a string snapshot only, not a typed target register; there is no source
  storage/register/stack-slot snapshot other than `from_value_id` and optional
  immediate; no explicit destination slot id accompanies destination stack
  offsets.
- `AbiBindingRecord` exists and is populated from prepared ABI bindings in move
  bundles. It keeps phase, destination kind/storage kind, ABI index,
  destination register spelling, destination prepared stack-offset snapshot,
  execution site, and source prepared pointers. Gaps: prepared
  `destination_contiguous_width` and `destination_occupied_register_names` are
  not copied; destination register is string-only; no destination slot id or
  source value id is present, so it is weaker than `MoveRecord` as a movement
  record.
- `SpillReloadRecord` exists and is populated from prepared regalloc
  spill/reload ops. It keeps value id, op kind, execution site, register bank,
  register spelling, slot id, prepared stack-offset snapshot, and source
  prepared pointer. Gaps: prepared `contiguous_width` and
  `occupied_register_names` are not copied; it has register bank but no
  prepared register class; scratch authority is implicit in the register name
  rather than linked to `TargetRegisterRecord`; no size/alignment/lifetime facts
  for the structured spill slot are present.
- `ParallelCopyRecord` exists and is populated from prepared control-flow
  parallel-copy bundles. It keeps predecessor/successor labels, execution site,
  optional execution block, cycle flag, move count, step count, and source
  prepared pointer. Gaps: per-move source/destination values, carrier kind,
  storage name, per-step kind, move index, and cycle-temp use remain only behind
  `source_bundle`; the target record itself is a summary rather than a direct
  consumable copy plan.

Stack-offset authority wording/status:

- Current module records consistently use `*_stack_offset_is_prepared_snapshot`
  or `offset_is_prepared_snapshot`, and builders set those flags when prepared
  offsets are present. Existing focused tests check representative operand,
  frame-slot, ABI-binding, and spill/reload prepared-snapshot flags.
- `ALLOCATION_CONTRACT.md` clearly says rendered stack offsets are not
  allocation authority and frame layout owns final offsets later. No immediate
  wording fix is required in the contract for Step 2, but code/docs should keep
  saying these fields are prepared snapshots only.

Markdown consumer notes:

- `codegen/mod.md`, `alu.md`, `memory.md`, `calls.md`, `returns.md`,
  `prologue.md`, `records.md`, and several specialized shards already warn
  that later slices must consume the allocation result/reserved MIR scratch
  policy rather than allocate registers, invent spills, or recover from
  assembly text.
- `BIR_PREPARED_GAP_LEDGER.md` is stale in its target-local table: it still says
  no AArch64 MIR container and no move/spill/reload records exist, even though
  `module::Module`, `FunctionRecord`, `OperandRecord`, `TargetRegisterRecord`,
  `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`, and
  `ParallelCopyRecord` now exist as prepared snapshots.

## Suggested Next

Proceed to Step 2 with the smallest code/docs target:

- In `module/module.hpp` and `module/module.cpp`, first tighten
  `TargetRegisterRecord` and `OperandRecord` by adding explicit representation
  for allocation-result location kind/authority, future virtual-register
  placeholders and owner, reserved-scratch versus long-lived-home status, and
  structured spill-slot metadata that stays separate from prepared stack-offset
  snapshots.
- Include the narrow move/spill register metadata copy if it falls naturally
  with the register-record work: propagate prepared contiguous width and
  occupied-register names into `MoveRecord`, `AbiBindingRecord`, and
  `SpillReloadRecord`, or intentionally leave that for Step 3/Step 4 if Step 2
  should remain only value-home/register-location records.
- After code changes, update focused module tests that assert physical register,
  spill-slot, non-register, reserved-scratch, and future/deferred location
  representation without assembly text.

## Watchouts

- Do not select or emit final `ldr`, `str`, `mov`, call, return, prologue, or assembly forms.
- Do not treat prepared stack offsets as final frame-layout authority.
- Do not recover allocation facts from rendered names, printed BIR, LIR text, parsed assembly, or legacy examples.
- Split missing shared prepared carriers into a separate open idea instead of patching around them locally.
- Keep `BIR_PREPARED_GAP_LEDGER.md` edits for Step 5 unless the supervisor
  explicitly wants a docs-only ledger correction earlier; Step 2 can rely on
  `todo.md` for the stale-ledger finding.
- Avoid treating string register spellings as enough typed authority for later
  machine-node selection; they are still prepared/display snapshots until a
  typed AArch64 register reference is carried.
- `ParallelCopyRecord` currently exposes only counts plus `source_bundle`.
  Later movement work should decide whether direct per-step target records are
  needed before instruction selection consumes it.

## Proof

Inspection-only packet. No build required and no `test_after.log` produced.
