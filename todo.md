Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Select the Next Semantic Owner

# Current Packet

## Just Finished

Completed Step 2 by sampling the focused remaining runtime-mismatch
representatives and selecting the next focused owner.

Focused subset:

- `00159`, `00164`, `00168`, `00169`, `00172`, `00183`, `00193`, `00202`,
  and `00217` were rerun under the delegated timeout-safe subset.
- All 9 failed as `RUNTIME_MISMATCH`, matching the broad post-292 inventory
  bucket.
- No generated runtime process remained after the run.

Selected semantic owner:

- AArch64 side-effecting expression and control-value publication authority.
- Starter representatives: `00164`, `00183`, and `00202`.
- Supporting nearby representative for early owner-boundary sampling: `00169`.

Why this owner is ready:

- `00164` shows expression values with side effects or control-derived results
  not being published to the later `printf` consumers. Examples include
  `(y = c + d)`, logical `&&`/`||`, comparison values, and arithmetic values;
  generated assembly prints stale or unrelated values such as `11`,
  callee-return values, or unmaterialized comparison results.
- `00183` shows conditional-expression branch results losing the selected
  value. The loop counter is printed instead of the selected `Count*Count` or
  `Count*3` result.
- `00202` shows compound-assignment side effects not updating both the
  expression result and the assigned object. Generated assembly stores from an
  unrelated stack slot, so `bob = jim *= 2` leaves `bob` and later `jim`
  stale.
- `00169` is useful as a boundary probe because the nested-loop middle value is
  loaded, then clobbered by format-pointer setup before the variadic call
  consumes it. It overlaps call-argument placement, so it should be supporting
  proof, not the first acceptance case.

Expected focused-idea proof shape:

- Start with backend-level proof that side-effecting expression results
  publish authoritative values to their consumers without relying on stale
  scratch or callee-saved registers.
- Runtime starter subset:
  `c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$`.
- Then sample the boundary case
  `c_testsuite_aarch64_backend_src_00169_c$` to make sure the repair does not
  merely handle one expression form and does not reopen closed call-argument
  authority by accident.
- Broader nearby sampling can add `00172` and `00217` only after pointer and
  aggregate/address-specific behavior is explicitly separated from the
  side-effect/control owner.

## Suggested Next

Step 3 should ask the plan owner to split a focused idea for AArch64
side-effecting expression and control-value publication authority, then switch
the active lifecycle state to that idea before implementation begins.

## Watchouts

- Do not implement fixes under this umbrella idea.
- Do not reopen closed AArch64 owners without contradictory generated-code
  evidence.
- Rejected or deferred closed-owner overlap:
  - `00159` and `00168` both show scalar parameter/call-return problems in
    generated code (`myfunc`/`factorial` consume stale parameter or return
    registers). They contradict closed-owner expectations enough to record as
    overlap, but they are not the cleanest next split while the side-effect
    owner has cleaner non-call starters.
  - `00193` switch dispatch compares `w13` instead of the incoming argument
    register, so it is scalar parameter/control overlap rather than a clean
    switch-only owner.
- Rejected or deferred pointer/aggregate/address bucket:
  - `00172` proves pointer compare/result publication is still broken, but it
    should be split only after scalar side-effect/control expression results
    are not masking the boolean publication path.
  - `00217` mixes global address/local pointer initialization, pointer
    arithmetic, compound assignment through a pointer, and string storage.
    It is too compounded for first proof but should be sampled after the
    cleaner side-effect owner.
- Deferred buckets outside this owner: printer/admission failures, timeout
  cases `00132` and `00220`, floating/conversion/string-only cases, and broad
  aggregate ABI or function-pointer behavior.

## Proof

No broad scan was run and no root proof logs were written.

Focused proof command:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00159|00164|00168|00169|00172|00183|00193|00202|00217)_c$' -j 4 --timeout 5 --output-on-failure; } > /tmp/c4c_aarch64_inventory_step2_runtime_subset.log 2>&1
```

Result: failed as expected for inventory sampling, 0/9 passing with all nine
selected representatives reported as `RUNTIME_MISMATCH`.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' > /tmp/c4c_aarch64_inventory_step2_runtime_subset.pgrep || true
```

Result: `/tmp/c4c_aarch64_inventory_step2_runtime_subset.pgrep` is empty.

Artifacts inspected:

- `/tmp/c4c_aarch64_inventory_step2_runtime_subset.log`
- `/tmp/c4c_aarch64_inventory_step2_runtime_subset.pgrep`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00159.c.s`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00164.c.s`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00168.c.s`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00169.c.s`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00172.c.s`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00183.c.s`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00193.c.s`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00202.c.s`
- `build-aarch64-scan/c_testsuite_aarch64_backend/src/00217.c.s`
