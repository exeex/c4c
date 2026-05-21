Status: Active
Source Idea Path: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Localized Starting-State Rule

# Current Packet

## Just Finished

Plan Step 2 repaired the visible starting-state stale preserve edge for
expanded global-array select-store lowering. Pending stack-home publications
for later `StoreGlobal` nodes now re-read direct `LoadGlobal` producers from
current memory instead of consuming their stale prepared stack homes. The
focused dispatch test now covers a later fixed-offset global store whose
non-selected arm preserves through a `LoadGlobal` appearing after an earlier
store; it rejects the stale `[sp, #40]` preserve reload before publishing the
later stack home.

The `00181.c` starting state is now correct: `A: 1 2 3 4`, `B: 0 0 0 0`,
`C: 0 0 0 0` with the slice narrowed back to the aligned direct `LoadGlobal`
current-memory select-store repair. The unguarded materialized
pointer-addressed `StoreLocal` fallback from the reviewed diff was removed
from this packet.

## Suggested Next

Route the residual pointer-addressed store gap as a separate owner before
using it to claim more `00181.c` progress. With that fallback removed, the
starting-state select-store repair still holds, but the later Hanoi moves do
not update the arrays.

## Watchouts

- The delegated proof is not green. `c_testsuite_aarch64_backend_src_00181_c`
  still fails after the starting-state fix, but the actual output now begins
  with the correct starting state.
- With the pointer-addressed `StoreLocal` fallback removed, every subsequent
  `00181.c` state remains at the starting values. The prior first-move advance
  therefore depended on that broader, split-worthy fallback.
- `00170`/`00189` and the named backend contracts still pass in the delegated
  subset.

## Proof

Ran the delegated proof command after the Step 2 repairs:

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: build completed; 6 of 7 tests passed. The only failure is
`c_testsuite_aarch64_backend_src_00181_c`; its starting state is correct, and
the later moves remain unchanged after removing the broader pointer-addressed
store fallback. Proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
