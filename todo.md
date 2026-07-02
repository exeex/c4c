Status: Active
Source Idea Path: ideas/open/523_bir_route2_select_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Close-readiness bookkeeping

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: recorded close-readiness notes for the route2
select-chain body extraction.

Final route disposition:

- Route2 select-chain value-record bodies and
  `route2_find_direct_global_dependency` moved to
  `src/backend/bir/bir_route2.cpp`.
- Public route2 declarations and public record types stayed in `bir.hpp`.
- Route6 still consumes route2 through public
  `route2_select_chain_value_record`; it does not reach into route2-private
  implementation details.
- No direct-global dependency classification, route6 publication policy, call
  ABI behavior, tests, or expectations were changed for this extraction.

Accepted validation and baseline disposition:

- Step 3 proof passed: `git diff --check`, default build, and backend subset
  with `345/345` tests.
- Supervisor regression guard passed with before `345/345`, after `345/345`,
  no new failures, and no new tests over 30 seconds.
- The accepted `test_after.log` proof was rolled forward to `test_before.log`.
- The hook-produced `test_baseline.new.log` candidate was rejected and deleted
  because it introduced failing `string_authority_guard` results versus
  `test_baseline.log`; the accepted baseline reference remains
  `test_baseline.log`.

No separate cleanup initiative was discovered. Source idea 523's acceptance
criteria appear satisfied subject to supervisor acceptance and plan-owner
lifecycle review.

## Suggested Next

Hand off to the plan owner for lifecycle close review if the supervisor accepts
the route2 slice.

## Watchouts

No residual route2-specific risks are known for this plan. Keep lifecycle close
review scoped to idea 523; any future route extraction or public header split
should remain separate work.

The rejected `test_baseline.new.log` must not be treated as accepted baseline
state; `test_baseline.log` remains the baseline reference.

## Proof

No new build or ctest was required for this close-readiness bookkeeping packet.

Recorded accepted prior proof:

```sh
git diff --check
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

Result: build passed, backend subset passed with `345/345` tests.

Recorded supervisor regression guard: before `345/345`, after `345/345`, no new
failures, no new tests over 30 seconds, and `test_after.log` rolled forward to
`test_before.log`.

This packet's local validation: `git diff --check -- todo.md`.
