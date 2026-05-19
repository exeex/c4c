Status: Active
Source Idea Path: ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Repair Condition-Safe Select-Chain Materialization

# Current Packet

## Just Finished

Step 2.1 - Fence The Partial Route And Select Policy: fenced the dirty partial
implementation into a condition-safe salvage slice. Salvaged pieces: prepared
scalar load sources with `base=global_symbol` now become symbol-backed memory
operands, symbol-backed control operands can emit `adrp/add/ldr`, and move
materialization can load prepared memory/immediates directly into an explicit
target register. Unwound pieces: the public `lower_scalar_value_to_register`
escape hatch, recursive select/cast value lowering, function-wide producer
lookup, direct call-argument ABI destination materialization, same-block select
producer expansion in `dispatch.cpp`, and the before-call loop that tried to
materialize call arguments immediately before retargeting moves. All
call-retarget expansion from the partial route was removed from the dirty
implementation.

## Suggested Next

Delegate Step 2.2 only after accepting this fenced baseline. The next packet
should implement a deliberate condition-safe select-chain materialization
owner for direct-global values consumed by calls, with explicit NZCV lifetime,
scratch/storage, ABI-destination, and producer-order contracts.

## Watchouts

- Reviewer found no testcase-overfit in the direction, but found a blocking
  stale-NZCV hazard in the now-unwound recursive select route: recursive select
  materialization could emit a nested compare between an outer compare and its
  outer `csel`.
- The fenced implementation is no longer expected to preserve the temporary
  `%t42`/`%t50` improvement from the unsafe route. Current inspection after
  fencing shows `[sp, #632]`, `[sp, #1064]`, `[sp, #1352]`, and `[sp, #1496]`
  are all still present; this is an intentional unwind to remove unsafe
  recursive lowering before Step 2.2.
- Do not reintroduce broad call-boundary retargeting or recursive select
  lowering until nested select lowering either materializes operands before the
  outer compare or recomputes the outer compare immediately before the outer
  `csel`.
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

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00(064|139|205)_c' > test_after.log 2>&1`.
The build succeeded after relinking the backend. CTest returned nonzero because
`00205` still has a runtime output mismatch. `00064` and `00139` passed.
`00205` completed in 0.06s, preserving the non-timeout
behavior, and generated assembly still uses legal `sxtw x9, w13` spelling.
`test_after.log` is the proof log.

Supervisor validation for the accepted Step 2.1 fenced slice: broader backend
before/after guard used `ctest --test-dir build -j --output-on-failure -R
'^backend_'`; before passed 139/139, after passed 139/139, no new failures or
slow tests; the strict checker returned nonzero only because pass count did not
strictly increase.
