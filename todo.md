Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Establish I128 Pair Or Memory Carrier Authority

# Current Packet

## Just Finished

Step 2 added explicit prepared/shared i128 carrier authority before any AArch64
i128 target selection.

Completed work:

- Added `PreparedI128Carrier` state on `PreparedBirModule` with function/value
  identity, carrier kind, source type, low/high lane order, lane width, total
  size/alignment, register-pair provenance, memory-backed slot/offset facts,
  and missing-fact diagnostics.
- Populated carriers from regalloc plus storage-plan authority after storage
  plans are published. Register-pair carriers require explicit width-2
  occupied-register facts and register bank/class authority; memory-backed
  carriers preserve stack slot plus low/high byte offsets.
- Added prepared-printer output for `prepared-i128-carriers`, including
  low/high lane facts and missing diagnostics.
- Added focused prepared tests for memory-backed i128 carriers, explicit
  register-pair carriers, and fail-closed diagnostics for incomplete
  register-pair authority.

## Suggested Next

Execute Step 3 from `plan.md`: select i128 transport nodes from complete
prepared i128 carrier facts. Start with pair-to-pair and memory/pair transport
records only; keep arithmetic, shifts, comparisons, helper calls, and printer
output deferred.

## Watchouts

- Do not create a local i128 allocator in AArch64 codegen.
- Do not infer low/high pair homes from rendered register names or fixed
  `x0`/`x1` accumulator conventions.
- Do not lower i128 as scalar i64 or claim progress through named testcase
  shortcuts.
- Do not treat generic `contiguous_width == 2` or
  `occupied_register_names.size() == 2` as semantic i128 low/high authority.
- Consume `PreparedI128Carrier` fields directly; do not rederive pair order in
  AArch64 selection from rendered register names, numeric adjacency, or fixed
  ABI registers.
- ABI/helper-call pair facts are not complete in this packet. If transport
  selection needs call-result or helper boundary pair policy, stop and report
  the missing prepared policy instead of inventing it in target codegen.
- Do not resurrect legacy helper conventions from `codegen/i128_ops.md`; helper
  calls need structured argument/result/clobber records first.
- Keep binary128/F128 and float/i128 conversions separate until helper-call
  facts carry explicit register-bank transition authority.

## Proof

Proof command run:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepare_stack_layout|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch)$') > test_after.log 2>&1`

Result: passed, 6/6 selected tests green. Proof log: `test_after.log`.

Supervisor full-suite acceptance also passed: `(cmake --build build -j2 &&
ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`, with
regression guard accepted against full-suite `test_before.log`; 3167/3167
before and 3167/3167 after.
