Status: Active
Source Idea Path: ideas/open/199_full_suite_baseline_string_authority_timeout_attribution.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove the Final Baseline Decision

# Current Packet

## Just Finished

Completed Step 5: Prove the Final Baseline Decision.

Narrow guard and isolated test result:
- `scripts/string_authority_guard.py` passed with
  `231 classified declaration-level hits`.
- `c_testsuite_aarch64_backend_src_00040_c` passed in isolation, confirming the
  Step 2 timeout/noise case is not a current isolated failure.

Full-suite baseline candidate:
- `scripts.plan_review_state.refresh_test_baseline_candidate(...)` generated
  fresh candidate `test_baseline.new.log`.
- Candidate baseline commit is
  `42fbc68e851f1dc929097fda1c91da182aac17c7`.
- Candidate baseline subject is
  `Classify AArch64 indirect callee fallback`.
- Candidate result is `<full-suite>` with `3428/3428` tests passing.

Baseline decision:
- The accepted `test_baseline.log` records a `<full-suite>` `3428/3428` pass.
- The fresh `test_baseline.new.log` also records a `<full-suite>` `3428/3428`
  pass, so it is non-regressive against the accepted baseline by status and
  pass count.
- Supervisor accepted `test_baseline.new.log` through
  `scripts/plan_review_state.py accept-baseline`; canonical `test_baseline.log`
  now records the current-HEAD `3428/3428` baseline.

## Suggested Next

Plan owner should decide whether this completed runbook closes the source idea.

## Watchouts

- `test_after.log` includes the generated candidate contents after the proof
  command prints `BASELINE_CANDIDATE test_baseline.new.log`.
- `test_baseline.new.log` has been consumed by supervisor acceptance.

## Proof

Ran the supervisor-delegated Step 5 proof and captured all output in
`test_after.log`:

```sh
(
  set -o pipefail
  python3 scripts/string_authority_guard.py && \
  ctest --test-dir build --output-on-failure -R '^c_testsuite_aarch64_backend_src_00040_c$' && \
  python3 - <<'PY'
from pathlib import Path
import scripts.plan_review_state as prs
candidate = prs.refresh_test_baseline_candidate(Path('test_baseline.log'), '')
print(f'BASELINE_CANDIDATE {candidate}')
print(Path(candidate).read_text())
PY
) > test_after.log 2>&1
```

Result: passed. `test_after.log` records the guard pass, the isolated `00040`
CTest pass, `BASELINE_CANDIDATE test_baseline.new.log`, and the full-suite
candidate result `100% tests passed, 0 tests failed out of 3428`.
