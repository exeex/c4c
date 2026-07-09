Status: Active
Source Idea Path: ideas/open/641_aggregate_global_object_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Boundaries And Reclassify Spillover

# Current Packet

## Just Finished

Step 4 recorded boundary proof after commit `075bc612a` (`Add aggregate global
byte-storage RV64 consumer`). The accepted `test_before.log` and current
`build/rv64_gcc_c_torture_backend/*/case.log` files show that
`src/complex-7.c` advanced past the old aggregate-global F32 blocker and now
stops later in `check_float` at
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering; function=check_float; block=entry; block_index=0;
instruction_index=27; instruction_kind=LoadLocalInst; owner=float %t4`.

That new `float %t4` spillover is outside idea 641 global-object
materialization. It belongs to local/frame or aggregate stack-home local-memory
ownership: the already-materialized aggregate global lane has been copied into a
local object, and the next missing consumer is a local `LoadLocalInst`, not
prepared global-symbol memory access.

Boundary rows retained their non-641 owners:
`src/pr60822.c` still stops on `unsupported_global_data` for supported prepared
global-memory facts; `src/pr88739.c` still stops on local memory authority;
`src/pr49073.c` still stops on local memory authority after its
destination/source fan-in ownership; and `src/pr60017.c` still stops on call ABI
or sret stack-home ownership before aggregate-global materialization can own the
row.

## Suggested Next

Next coherent packet: Step 5 final lifecycle review. Decide whether idea 641 is
complete with the accepted aggregate global byte-storage consumer slice, should
close with the local/frame `LoadLocalInst` spillover assigned elsewhere, or
needs a replacement runbook.

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
  `LoadLocalInst` fragment gap at `check_float` instruction index `27`; that is
  spillover, not unfinished idea 641 global-object materialization.
- `src/pr49073.c` has aggregate global lanes in the dump, but its first owner
  is destination/source fan-in authority. Counting it as idea 641 progress would
  be route drift.
- `src/pr60017.c` and `src/pr88739.c` have missing or contradicted local/sret
  layout authority. They should not be used to justify RV64 aggregate-global
  materialization.

## Proof

No new proof command was run for this evidence-only packet. Per delegation, this
classification used the accepted `test_before.log` from commit `075bc612a` and
the current case logs under `build/rv64_gcc_c_torture_backend/`; no
`test_after.log` or new root-level `.log` file was created.

Relevant evidence:

- `test_before.log` records the allowlist probe result as `total=5 passed=0
  failed=5`.
- `build/rv64_gcc_c_torture_backend/src_complex-7.c/case.log` records the new
  `check_float` instruction index `27` `LoadLocalInst` blocker for
  `float %t4`.
- `build/rv64_gcc_c_torture_backend/src_pr60822.c/case.log` records
  `unsupported_global_data: RV64 object route requires supported prepared
  global memory facts`.
- `build/rv64_gcc_c_torture_backend/src_pr88739.c/case.log` and
  `build/rv64_gcc_c_torture_backend/src_pr49073.c/case.log` record
  `unsupported_local_memory_access`.
- `build/rv64_gcc_c_torture_backend/src_pr60017.c/case.log` records
  `unsupported_call_abi`.
