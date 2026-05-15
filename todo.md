Status: Active
Source Idea Path: ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Broader Backend Validation And Closure Readiness

# Current Packet

## Just Finished

Plan Step 6 - Broader Backend Validation And Closure Readiness completed after
plan-owner review accepted the builtin-address ownership boundary.

Step 5 boundary status:

- Existing dispatch and machine-printer tests already prove complete DC CVAU
  cache-maintenance and hint-yield carriers remain non-selected and unprinted.
- No selected-machine records, dispatch cases, or printer spelling were added
  for cache-maintenance, pause/hint, or builtin-address families.
- No additional Step 5 implementation packet is needed unless the broader
  backend validation exposes a regression.

Ownership decision:

- No concrete non-materialization builtin-address representative is currently
  owned by this intrinsic-carrier route.
- Existing direct, GOT, string, and label address cases already flow through
  structured address-materialization carriers and selected AArch64 address
  materialization records.
- Existing thread-local address cases already flow through TLS materialization
  facts, including the local-exec thread-pointer-relative `tpidr_el0` sequence.
- Those cases must stay with address-materialization or TLS ownership instead
  of being reclassified as builtin-address intrinsic carrier support.
- Builtin-address support should not be claimed from archived legacy intrinsic
  text, generic call plans, scratch-register conventions, or rendered assembly.

No accepted builtin-address carrier was added. This route should not claim
builtin-address intrinsic-carrier support beyond the recorded ownership
boundary unless a later source idea names a concrete non-materialization
representative.

## Suggested Next

Ask the plan owner to close
`ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md`. No split is
needed right now because builtin-address has no concrete non-materialization
representative in this source idea.

## Watchouts

- Keep this route limited to cache-maintenance, pause/hint, and
  builtin-address carrier authority.
- Do not add selected-machine records, dispatch support, or printer spelling in
  the carrier packets.
- DC CVAU and hint-yield support are carrier-only. Later machine-node routes
  must consume the structured carriers rather than rediscovering support from
  intrinsic names or rendered text.
- Do not treat prepared address materialization or TLS metadata as
  builtin-address intrinsic authority. Existing direct/GOT/TLS global, string,
  and label address materializations already have their own carrier and
  selected-machine routes.
- Builtin-address remains a recorded ownership boundary unless a later source
  idea names a concrete non-materialization representative.
- `llvm.aarch64.hint` malformed LIR cases can lower without producing
  `HintYield` authority in this harness; the Step 3 BIR test explicitly guards
  against producing a semantic carrier for unsupported immediate/result cases.

## Proof

Broader backend validation:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

Result: passed. Build was up to date and 139/139 backend tests passed.
Matching before/after backend logs passed the regression guard with no new
failures.

Log path: `test_after.log`.
