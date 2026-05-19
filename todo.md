Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Post-307 Backend-Regex Context

# Current Packet

## Just Finished

Step 1 from `plan.md` reconstructed the post-307 backend-regex context from
durable idea 295 notes, closed idea 307 notes, and the current focused
`test_before.log`. No tests were rerun and no implementation files were
inspected or changed.

The latest broad backend-regex inventory evidence available remains the
accepted post-306 umbrella inventory recorded in idea 295: `ctest -R backend`
selected 352 tests, passed 306, and left 46 failures, all under
`c_testsuite_aarch64_backend_*`; local backend/unit/CLI tests selected by
`-R backend` passed. That post-306 classification split focused idea 307 for
the `00182.c` illegal large scalar immediate assembler residual.

Focused idea 307 is now closed. Its durable closure note records commit
`4af6bc256` removing the illegal `mov x0, #1234567` assembly form by reusing
legal integer constant materialization for call-boundary scalar immediates.
The focused proof scope remained 1/2: `backend_aarch64_machine_printer`
passes, while `c_testsuite_aarch64_backend_src_00182_c` now fails as
`RUNTIME_MISMATCH`. The current `test_before.log` confirms that focused
post-307 state for `00182.c`.

Therefore `00182.c` has moved from the post-306 assembler/legalization bucket
back into the umbrella runtime mismatch bucket. It should not reopen idea 307
unless generated-code evidence shows illegal large scalar immediate forms still
reach AArch64 assembly printing.

## Suggested Next

Execute Step 2 from `plan.md` only after the supervisor decides the proof
shape. Because the only post-307 evidence on disk is focused closure evidence,
not a broad backend-regex inventory, Step 2 needs a supervisor-delegated fresh
bounded `ctest -R backend` capture before claiming a current classified
post-307 backend-regex failure inventory. If the supervisor intentionally
accepts stale broad counts, Step 2 can only classify from the post-306 352 /
306 / 46 baseline plus the focused `00182.c` delta, and must label that result
as reconstructed rather than freshly captured.

## Watchouts

- Do not implement repairs under this umbrella idea.
- Do not reopen closed owners 285 through 307 from failure counts alone.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or proof-log policy.
- Do not run broad runtime tests without supervisor-directed timeout handling
  and stale-process cleanup.
- Treat the focused idea 307 proof evidence as closure evidence, not as a
  fresh broad backend-regex inventory.
- Root `test_after.log` is currently absent in this workspace; durable idea
  295 and closed idea 307 record that the focused close-time regression guard
  passed, but the on-disk focused evidence available to this packet is
  `test_before.log`.

## Proof

Lifecycle/log context reconstruction only. Per packet instructions, no build
or tests were run. Evidence used: `ideas/open/295_backend_regex_failure_family_inventory.md`,
`ideas/closed/307_aarch64_large_scalar_immediate_materialization.md`, and
focused `test_before.log`; `test_after.log` was requested as part of the
current focused pair but is not present on disk.
