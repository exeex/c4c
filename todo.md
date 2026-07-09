Status: Active
Source Idea Path: ideas/open/635_prepared_branch_stack_clobber_safety_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Admit Only Explicitly Safe Branch Stack Loads

# Current Packet

## Just Finished

Step 4 probed the RV64/prealloc consumer boundary for the seven delegated
representative rows. RV64 is reading prepared branch stack-load authority
records and rejecting rows whose selected pointer source freshness is present
but whose prepared clobber-safety authority is still not `Available`.

Prepared dumps and the delegated allowlist run show two rows have already moved
outside this packet:
- `src/20001017-1.c`: prepared pointer `lhs` authority for `bug` is
  `available`; the backend stops later at `unsupported_call_abi` in `main`.
- `src/20000314-3.c`: prepared pointer `rhs` authority for `attr_rtx` is
  `available`; the backend stops later at `unsupported_terminator_fragment`.

Five rows remain in-scope selected-freshness rows that still fail closed at
`missing_stack_clobber_safety`: `src/loop-2e.c`, `src/pr39100.c`,
`src/20140828-1.c`, `src/20080519-1.c`, and `src/20050125-1.c`. Focused dumps
are under `build/agent_state/635_step4_*`, with the rollup in
`build/agent_state/635_step4_branch_authority_probe.tsv`.

## Suggested Next

The next coherent packet should return to the producer side or split a follow-up
for call-preservation-aware branch stack clobber safety. For example,
`src/loop-2e.c` has an intervening same-module call that explicitly preserves
`%t23` in the same stack slot, but the current prepared clobber-safety proof
still treats call/helper instructions as fail-closed.

## Watchouts

No consumer change was made. Admitting the five remaining rows from RV64 would
require bypassing prepared `Available` authority or inferring safety from stack
homes/offsets/final shape, which is out of scope and would weaken the
fail-closed rule. `src/20001017-1.c` and `src/20000314-3.c` should not be
counted as Step 4 consumer blockers because their selected pointer branch
authority is already available.

## Proof

`cmake --build --preset default && ALLOWLIST=build/agent_state/635_step1_branch_clobber_safety.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: failed as expected for this blocked consumer-only packet:
`total=7 passed=0 failed=7`. Proof log: `test_after.log`.
