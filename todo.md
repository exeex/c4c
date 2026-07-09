Status: Active
Source Idea Path: ideas/open/641_aggregate_global_object_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Narrow Producer Or Consumer Repair

# Current Packet

## Just Finished

Step 3 now has a width-aware RV64 byte-storage aggregate global-symbol consumer
for the idea 641 lane family: complete-authority 4-byte F32 lanes and 16-byte
aggregate lanes. Both widths require explicit prepared global-symbol identity,
base-plus-offset addressing, exact requested range, complete object extent,
`byte_storage_aggregate` layout authority, and `proven_in_bounds` facts before
object emission composes legal RV64 memory operations.

Focused unit coverage now includes the real earlier F32 lane shape from
`src/complex-7.c`: a prepared F32 aggregate global-symbol load into an FPR,
followed by a prepared local lane store. The selected 16-byte lane path is also
covered through the F128 store-local publication route, plus the existing I128
stack-home/global-store round trip. Fail-closed tests still reject missing,
misaligned, scalar-layout, and out-of-bounds byte-storage aggregate facts.

The delegated positive row advanced past the previous F32 global-data blocker.
`src/complex-7.c` now stops later in `check_float` at
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering; function=check_float; block=entry; block_index=0;
instruction_index=27; instruction_kind=LoadLocalInst; owner=float %t4`.
That is a local/frame load consumer boundary, not the prior aggregate global
consumer gap.

## Suggested Next

Next coherent packet: repair the new `src/complex-7.c` local/frame load
consumer blocker for the F32 copied aggregate lane, if the supervisor agrees it
is still inside the idea 641 route to reach the selected 16-byte lanes. Keep the
current global consumer gates unchanged and re-run the same allowlist proof.

## Watchouts

- Do not treat aggregate global-object materialization as scalar direct
  `addr @symbol` local-memory support.
- Do not add testcase-shaped handling for the named rows or specific
  aggregate offsets.
- Do not infer aggregate lanes, byte ranges, or destination authority from
  final assembly layout or source object spelling.
- The current F32 support is intentionally limited to complete byte-storage
  aggregate global-symbol lanes. Do not convert this into scalar direct-global
  support or general F32 global policy without separate authorization.
- `src/complex-7.c` has advanced from global-data rejection to a local
  `LoadLocalInst` fragment gap at `check_float` instruction index `27`; do not
  spend the next packet rediscovering the old F32 global diagnostic.
- `src/pr49073.c` has aggregate global lanes in the dump, but its first owner
  is destination/source fan-in authority. Counting it as idea 641 progress
  would be route drift.
- `src/pr60017.c` and `src/pr88739.c` have missing or contradicted local/sret
  layout authority. They should not be used to justify RV64 aggregate-global
  materialization.

## Proof

Current proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' && ALLOWLIST=build/agent_state/641_step1_aggregate_global.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh) > test_after.log 2>&1`

Result: build and `backend_riscv_object_emission` passed, then the allowlist
probe failed with `total=5 passed=0 failed=5`. `src/complex-7.c` advanced past
the previous 4-byte F32 aggregate global-symbol diagnostic and now stops at the
later `LoadLocalInst` fragment gap for `float %t4` in `check_float`. Boundary
rows remained fail-closed: `src/pr49073.c` and `src/pr88739.c` stop on local
memory authority, `src/pr60017.c` stops on call ABI, and `src/pr60822.c` stops
on supported prepared global-memory facts. See `test_after.log`.
