Status: Active
Source Idea Path: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reconcile memory.md And Close Shard Ownership

# Current Packet

## Just Finished

Step 5 reconciled `src/backend/mir/aarch64/codegen/memory.md` against the
compiled memory owner and deleted the shard after classifying its durable
content here.

Represented by the compiled memory owner:

- Prepared memory entry points: `make_prepared_memory_operand_record` for local
  loads, local stores, global loads, and global stores preserve prepared
  address facts, byte offsets, sizes, alignments, address spaces, volatility,
  base identity, and result/stored value identity.
- Selected ordinary memory nodes:
  `make_prepared_frame_slot_load_memory_instruction_record`,
  `make_prepared_store_memory_instruction_record`, `make_memory_instruction`,
  and `lower_memory_instruction` preserve the accepted load/store subset as
  structured machine nodes with memory defs/uses and memory read/write plus
  volatile side effects.
- Address families currently supported by selected ordinary memory nodes are
  direct frame-slot loads/stores and register-backed pointer-value stores; the
  pointer-value path records the pointer value home and storage-plan register
  instead of reviving legacy fixed `x9` address scratch.
- F128 memory transport entry points:
  `lower_f128_transport_instruction` and
  `make_prepared_f128_carrier_transport_record` keep binary128 loads/stores as
  explicit prepared-carrier memory transports and reject missing or incomplete
  carrier facts.
- I128 memory transport follows the same prepared-carrier memory path through
  `lower_i128_transport_instruction` and
  `make_prepared_i128_carrier_transport_record`.
- Memory spelling now has a memory-owned `memory_address` helper in
  `memory.cpp`/`memory.hpp`; printer paths for inline-asm memory operands,
  spill/reload slots, ordinary memory nodes, I128 transport, and F128
  transport route through that helper or existing structured printer records.
- Width and sign behavior for ordinary scalar memory is represented at the
  typed register/storage boundary: register operands are built from prepared
  value homes, storage-plan entries, and BIR type views. No parsed mnemonic text
  is semantic authority for selected memory nodes.

Represented by existing shared contracts rather than `memory.md`:

- Scratch ownership, value homes, spill slots, and reload/store pseudo
  placement are governed by `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`.
  The legacy `x0`, `x1`, `x9`, `x10`, `x11`, `x12`, and `x17` conventions are
  archived examples only.
- Dynamic stack, frame-slot, memory-return, call-time stack, and frame-pointer
  sensitive behavior are governed by
  `src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`.
- Spill/reload spelling remains tied to `module::SpillReloadRecord` and shared
  machine-printer records; general memory lowering does not create spill/reload
  operations locally.

Explicitly deferred carrier/feature gaps from the deleted shard:

- Legacy entry points `emit_store_impl`, `emit_load_impl`,
  `emit_store_with_const_offset_impl`,
  `emit_load_with_const_offset_impl`,
  `emit_typed_store_to_slot_impl`, `emit_typed_load_from_slot_impl`,
  `emit_load_ptr_from_slot_impl`, `emit_typed_store_indirect_impl`, and
  `emit_typed_load_indirect_impl` are represented only where prepared ordinary
  memory records currently support them. Missing direct load variants,
  pointer-value loads, stack-slot-vs-pointer slot fallback behavior, and
  unresolved-address no-op behavior remain deferred until prepared carriers
  define them.
- Legacy entry points `emit_add_offset_to_addr_reg_impl`,
  `emit_add_imm_to_acc_impl`, `emit_slot_addr_to_secondary_impl`,
  `emit_gep_direct_const_impl`, and `emit_gep_indirect_const_impl` remain
  deferred. The current selected memory owner snapshots prepared byte offsets
  and base+offset capability but does not locally materialize GEP or large
  signed-offset add/sub/register sequences.
- Legacy dynamic-stack entry points `emit_round_up_acc_to_16_impl`,
  `emit_sub_sp_by_acc_impl`, `emit_mov_sp_to_acc_impl`,
  `emit_mov_acc_to_sp_impl`, and `emit_align_acc_impl` remain deferred to the
  dynamic-stack/frame contract and future carriers; the memory owner must not
  recover them from assembly-text shims.
- Legacy over-aligned alloca entry points
  `emit_alloca_aligned_addr_impl` and
  `emit_alloca_aligned_addr_to_acc_impl` remain deferred until prepared alloca
  alignment/address carriers exist for selected memory. The old `align - 1`
  add plus `-align` mask rule is recorded as the rebuild requirement.
- Legacy memcpy entry points `emit_memcpy_load_dest_addr_impl`,
  `emit_memcpy_load_src_addr_impl`, and `emit_memcpy_impl_impl` remain
  deferred. A future bytewise memcpy lowering must carry explicit source,
  destination, size, scratch, loop-label, alignment, and overlap semantics
  instead of assuming fixed `x9`/`x10`/`x11`/`w12` registers.
- Known rebuild risks from the shard are accounted for as either owner
  behavior or deferred blockers: preserve F128 helper-routed transport; keep
  direct, indirect, and over-aligned alloca address families distinct; preserve
  source values across address computation through allocation/scratch records;
  do not confuse direct stack slots with pointer slots; materialize large
  signed offsets and alignment masks through structured scratch authority;
  preserve loaded-result storage after loads; keep sign/zero extension visible
  through typed instruction-selection facts; invalidate stale accumulator-like
  cache state only if such a carrier is reintroduced; and do not widen memcpy
  without proving alignment and overlap assumptions.

## Suggested Next

Proceed to Step 6 validation or reviewer/plan-owner closure routing; no
remaining durable `memory.md` prose is stranded outside compiled owners,
shared contracts, or the deferred carrier notes above.

## Watchouts

This packet did not expand memory semantics and did not edit implementation
files. The deletion is honest because unsupported legacy stack/GEP/dynamic
stack/over-aligned alloca/memcpy behavior is explicitly retained here as
deferred carrier work rather than claimed as compiled behavior.

Broad printer dispatch and transport printer bodies still exist in
`machine_printer.cpp`, but the shared memory address spelling helper is
memory-owned. Atomic memory behavior is represented in `memory.cpp`, but
atomic-specific legacy content remains owned by the separate atomic shard.

## Proof

Ran exact delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: green. `test_after.log` contains the combined build and backend CTest
output, ending with 139/139 backend tests passed.
