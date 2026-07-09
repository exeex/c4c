Status: Active
Source Idea Path: ideas/open/629_prepared_return_destination_home_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Representative Rows

# Current Packet

## Just Finished

Step 5 re-ran the representative return rows after the Step 3/4 return
destination-home authority changes:

- `src/20001130-2.c`: failed with `[RV64_BACKEND_RUNTIME_MISMATCH]`,
  `clang_exit=0`, and `c4c_exit=Segmentation fault`. The row now reaches a
  linked executable instead of failing return-authority admission. The emitted
  `output_25` return paths return uninitialized saved registers or stack
  contents for string literal addresses (`s1`, `s2`, or `0(sp)`) rather than
  materialized `.rodata` homes, so the residual owner is local/global literal
  address publication or address-value preservation, not return
  destination-home authority.
- `src/20080719-1.c`: failed with `[RV64_BACKEND_RUNTIME_MISMATCH]`,
  `clang_exit=0`, and `c4c_exit=Segmentation fault`. The row now reaches a
  linked executable instead of failing return-authority admission. The emitted
  `xxx` return paths store and reload uninitialized registers (`s2`, `s1`, or
  `t0`) for static table addresses rather than materialized `.rodata` homes,
  so the residual owner is local/global table address publication or
  address-value preservation, not return destination-home authority.

Both representative rows are now out of scope for idea 629. The return
destination-home authority slice is close-ready from this reclassification.

## Suggested Next

Ask the plan owner to close idea 629, preserving the residual runtime
segfaults as out-of-scope local/global address publication or address-value
preservation work if follow-up tracking is needed.

## Watchouts

Do not broaden this idea into the residual runtime segfaults. They are after
return-authority admission and point at missing constant or table address
materialization/preservation, which is outside the return destination-home
authority boundary.

## Proof

Proof command:

`cmake --build --preset default && ALLOWLIST=build/agent_state/629_step5_return_authority.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/629_step5_return_authority.log 2>&1`

Result: build passed; focused scan returned nonzero with `total=2 passed=0 failed=2`.

Log path: `build/agent_state/629_step5_return_authority.log`.
