Status: Active
Source Idea Path: ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Focused Proof And Broader Safety Check

# Current Packet

## Just Finished

Step 2.3 - Focused Proof And Broader Safety Check: reran the final focused
proof for the accepted Step 2.2 repair. The Step 2.2 implementation remains the
focused direct-global select-chain materialization path for AArch64 call
arguments: it materializes same-block select chains rooted in prepared global
loads through reserved MIR scratches `x9`/`x10`, consumes predicate flags
immediately with branch materialization, records the selected value as an
emitted scalar, and lets the existing before-call move place the final value in
the ABI argument register. The value-materialization owner appears complete;
no later residual was exposed by the final focused proof.

## Suggested Next

Proceed to Step 3 residual handoff or closure recommendation. The next packet
should decide whether the active runbook can be closed for idea 305 based on
the green focused proof, resolved high stack-home reads, preserved timeout
repair, and preserved sign-extension legality.

## Watchouts

- The implemented select-chain route intentionally uses branch-based
  materialization for this call-site owner instead of recursive `csel`, because
  two reserved scratches are not enough to hold true value, false value, and a
  rematerialized compare operand for the deep `%t26`/`%t34` chains.
- The route is scoped by same-block producer order and by
  `select_chain_contains_direct_global_load`; it should not be widened into
  general call-retargeting without a dominance/order policy.
- The generated assembly no longer contains `[sp, #632]`, `[sp, #1064]`,
  `[sp, #1352]`, or `[sp, #1496]` reads, and keeps final ABI placement in the
  normal before-call move (`mov x3, x9` / `mov x2, x9`) after materialization.
- Preserve the idea 304 timeout repair: `00205` should complete quickly and
  must not return to the 5-second timeout path.
- Preserve the idea 303 legality repair: generated assembly must keep legal
  sign-extension spelling such as `sxtw x9, w13`.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, proof-log policy, or CTest registration.
- Do not special-case filename `00205`, exact stack offsets, case indexes,
  temporary labels, or source line numbers.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00(064|139|205)_c' > test_after.log 2>&1`.
The build was already up to date and the focused subset passed 3/3: `00064`,
`00139`, and `00205` all passed. `00205` completed in 0.05s, preserving the
non-timeout behavior. Generated assembly inspection found no `[sp, #632]`,
`[sp, #1064]`, `[sp, #1352]`, or `[sp, #1496]` reads, and legal
`sxtw x9, w13` spelling remains present. The repaired select-chain values are
materialized before the calls and placed into ABI argument registers with
`mov x3, x9` / `mov x2, x9`. `test_after.log` is the fresh proof log. The
prepared direct-global value-materialization owner appears complete; no later
residual needs handoff from this proof.

Supervisor validation for accepted Step 2.2: focused regression guard passed
with before 2 passed / 1 failed and after 3 passed / 0 failed, resolving
`c_testsuite_aarch64_backend_src_00205_c`; broader backend before/after guard
used `ctest --test-dir build -j --output-on-failure -R '^backend_'`, before
passed 139/139, after passed 139/139, no new failures or slow tests; the strict
checker returned nonzero only because the broader pass count did not strictly
increase.
