Status: Active
Source Idea Path: ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Fence The Partial Route And Select Policy

# Current Packet

## Just Finished

Plan-owner rewrite after reviewer-derived Step 2 route feedback: kept idea
305 active and split Step 2 around condition-safe select-chain materialization
before call consumption. No implementation files, test logs, or review
artifacts were edited.

## Suggested Next

Supervisor should decide whether the current uncommitted implementation diff
is salvaged, replaced, or unwound before delegating Step 2.1. The next executor
packet should fence the partial route first: preserve prepared direct-global
load materialization as the owner, then make the NZCV, scratch, ABI-register,
and producer-lookup policies explicit before extending recursive select
lowering.

## Watchouts

- Reviewer found no testcase-overfit in the current direction, but found a
  blocking stale-NZCV hazard: recursive select materialization can emit a
  nested compare between an outer compare and its outer `csel`.
- Preserve the semantic evidence from the incomplete route: prepared
  direct-global materialization removed the `[sp, #1352]` and `[sp, #1496]`
  call-read residuals, while `%t26` and `%t34` still fall back to `[sp, #632]`
  and `[sp, #1064]`.
- Do not extend the broad call-boundary retarget route until nested select
  lowering either materializes operands before the outer compare or recomputes
  the outer compare immediately before the outer `csel`.
- Use only reserved backend scratches or explicit stable temporary storage. Do
  not borrow unreserved registers to make the select chain fit.
- Treat outgoing ABI argument registers as final destinations, not hidden
  temporaries that can be clobbered while sibling operands are still live.
- Function-wide producer lookup must be dominance/order-safe or remain scoped
  to the focused direct-global materialization owner.
- Preserve the idea 304 timeout repair: `00205` should complete quickly and
  must not return to the 5-second timeout path.
- Preserve the idea 303 legality repair: generated assembly must keep legal
  sign-extension spelling such as `sxtw x9, w13`.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, proof-log policy, or CTest registration.
- Do not special-case filename `00205`, exact stack offsets, case indexes,
  temporary labels, or source line numbers.

## Proof

Lifecycle rewrite only; no build or CTest run. The last executor proof recorded
for the incomplete route was
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00(064|139|205)_c' > test_after.log 2>&1`,
with `00064` and `00139` passing and `00205` still failing output comparison
without timing out.
