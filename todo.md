Status: Active
Source Idea Path: ideas/open/353_aarch64_local_formal_frame_slot_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Formal To Local Slot Publication Boundary

# Current Packet

## Just Finished

Step 1 localized the AArch64 scalar formal-to-local publication boundary for
`00176` `partition` without implementation changes.

Evidence:

- Semantic BIR is correct at the local initialization boundary:
  `partition(i32 %p.left, i32 %p.right)` stores `%p.left` into
  `%lv.pivotIndex`, `%lv.index`, and `%lv.i`, then later loads those locals.
- Prepared BIR keeps the formal and local ownership split visible:
  `%p.left` has value home/storage `register x0`, `%p.right` has
  value home/storage `register x1`, while local frame objects are
  `%lv.pivotIndex` slot `#1` offset `0`, `%lv.pivotValue` slot `#2` offset
  `4`, `%lv.index` slot `#3` offset `8`, and `%lv.i` slot `#4` offset `12`.
  Prepared addressing for `partition` records `store_local %lv.pivotIndex`
  as `stored=%p.left frame_slot=#1`, `store_local %lv.index` as
  `stored=%p.left frame_slot=#3`, and `store_local %lv.i` as
  `stored=%p.left frame_slot=#4`.
- Generated AArch64 does not publish the incoming formal register into those
  local slots before local loads consume them: `partition:` begins with
  `ldr w9, [sp]` then `str w9, [sp, #16]`, later reloads stale local-slot
  data such as `ldr w13, [sp]`, `ldr w13, [sp, #12]`, and `ldr w13, [sp, #8]`
  along the `swap`/loop/return path instead of first storing `w0` into the
  local slots used for `pivotIndex`, `index`, and `i`.
- Runtime proof remains red at the representative: the focused subset still
  times out in `c_testsuite_aarch64_backend_src_00176_c`.

Owner boundary:

- The first bad owner is AArch64 store-local publication from a prepared
  register-backed formal into a prepared frame-slot-backed local, not BIR local
  lowering, stack-slot assignment, frame-slot addressing, branch label ordering,
  or call-boundary preservation. The likely Step 2/3 implementation surface is
  `src/backend/mir/aarch64/codegen/memory.cpp`:
  `make_store_memory_instruction_record`,
  `make_prepared_store_memory_instruction_record`, and the local-store path
  through `lower_memory_instruction`, plus any narrowly required interaction
  with `src/backend/mir/aarch64/codegen/dispatch.cpp`
  `lower_entry_formal_publications` and scalar-state recording. The repair
  should make stores like `bir.store_local %lv.*, i32 %p.left` consume the
  prepared formal register home (`w0`) and write the destination local frame
  slot before any `load_local` reads that slot.

## Suggested Next

Execute plan Step 2/first implementation-prep packet: add focused AArch64
coverage for a scalar fixed formal copied into one or more locals, read before
and after a call, and returned through a local-derived value. The test should
fail on the current generated behavior because the local slot is populated from
stale stack memory instead of the incoming formal register, and it should avoid
matching `00176`, `partition`, specific local names, stack offsets, or register
numbers.

## Watchouts

- Keep idea 352's repaired `.LBB90_6`/`.LBB90_7` branch/label path intact.
- Do not special-case `00176`, `partition`, one local name, one stack offset,
  one formal register, or one emitted instruction sequence.
- Do not broaden into variadic, byval, HFA, aggregate writeback, or broad frame
  layout without fresh first-bad-fact evidence.
- Prepared BIR already has the right semantic and addressing facts for this
  boundary; changing BIR local-slot creation or stack layout would be route
  drift unless a new first-bad fact disproves the current evidence.
- `lower_entry_formal_publications` records same-register incoming formals in
  scalar state without emitting a move when source and destination are both the
  ABI register. Step 3 should preserve that behavior while ensuring subsequent
  `store_local` lowering uses the register-backed formal as the store value.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_operand_resolution|backend_prepare_frame_stack_call_contract|backend_prepare_stack_layout|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00176_c)$' | tee test_after.log`

Result: build passed; 5/6 tests passed. The five backend guardrails passed and
`c_testsuite_aarch64_backend_src_00176_c` timed out. Proof log:
`test_after.log`.
