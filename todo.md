Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Post-309 Backend-Regex Context

# Current Packet

## Just Finished

Step 1 from `plan.md` reconstructed the usable post-309 backend-regex context
from idea 295, focused idea 309's closure note, and the current focused
`test_before.log` / `test_after.log` pair.

The accepted focused 309 proof moved
`c_testsuite_aarch64_backend_src_00189_c` from
`RUNTIME_NONZERO exit=Segmentation fault` in `test_before.log` to passing in
`test_after.log` on the same 5-test focused scope. Idea 295 and closed idea
309 also record supervisor local backend validation at
`ctest --test-dir build -j --output-on-failure -R '^backend_'` with 139/139
passing.

`00189.c` is removed from the umbrella residual set. No residual from the
closed indirect-call callee and argument preservation owner returns to idea
295 unless future generated-code or proof evidence contradicts that closure.
The remaining umbrella context is the parked pre-309 backend-regex residual
surface: direct-call shuffle, direct vararg, address-of-local, timeout,
runtime mismatch/crash, machine-printer/prepared-node, and semantic
`lir_to_bir` buckets.

Existing accepted evidence can support historical classification, closed-owner
boundaries, local backend non-regression context, and explicit exclusion of
`00189.c`. It cannot support a current complete post-309 `ctest -R backend`
failure count, a current residual filename list, or a new focused owner split
from broad c-testsuite buckets, because the canonical logs are focused 309
closure logs rather than a fresh broad backend-regex inventory.

## Suggested Next

Step 2 needs a supervisor-delegated fresh bounded backend-regex capture before
claiming the current residual buckets or splitting the next focused owner.
Use the existing accepted evidence only as a seed and subtract `00189.c` from
the old umbrella residual set.

## Watchouts

- This umbrella packet is classification only; do not implement repairs here.
- Do not reopen closed owners 285 through 309 from counts alone.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- Treat the last broad backend-regex count in idea 295, 352 selected / 306
  passed / 46 failed, as pre-309 historical context. It still includes the old
  `00189.c` residual and is not a current post-309 count.
- Treat the current `test_before.log` / `test_after.log` pair as focused idea
  309 closure evidence only. It selected 5 tests, not the full backend regex.
- Do not run a fresh broad backend regex command unless the supervisor
  delegates that proof command with timeout and stale-process guidance.

## Proof

No build or CTest was run for this classification-only packet. Read-only
evidence used: `ideas/open/295_backend_regex_failure_family_inventory.md`,
`ideas/closed/309_aarch64_indirect_call_argument_preservation.md`,
`ideas/closed/308_aarch64_extern_data_symbol_pic_address_formation.md`,
`test_before.log`, and `test_after.log`. `test_after.log` was not written.
