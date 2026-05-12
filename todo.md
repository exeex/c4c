Status: Active
Source Idea Path: ideas/open/192_hir_compile_time_rendered_registry_api_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Validate And Prepare Closure Evidence

# Current Packet

## Just Finished

Completed Step 5 - Validate And Prepare Closure Evidence.

Final acceptance evidence for idea 192:
- Selected route: direct consteval call lowering through
  `try_lower_consteval_call_expr`. This route has callee `Node*` metadata and
  was changed to require structured consteval identity before using rendered
  registry compatibility, so a complete structured miss cannot silently recover
  through stale rendered spelling.
- Focused proof: supervisor-supplied focused guard for `frontend_hir_tests` and
  `frontend_hir_lookup_tests` passed with before `2/2`, after `2/2`, and no new
  failures. The focused proof covers structured direct-call success, stale
  rendered registry fallback rejection after metadata miss, and preservation of
  diagnostics/display behavior.
- Broader proof: hook-produced full-suite baseline for commit `78f3cc95d` was
  accepted after comparing `test_baseline.log` and `test_baseline.new.log`;
  both reported `3137/3137` passing with no new failures. Canonical validation
  logs available for close review are `test_baseline.log`, `test_before.log`,
  and `test_after.log`.
- Remaining compatibility boundaries: rendered template-function and
  template-struct registries remain compatibility surfaces for deferred
  template carriers; rendered consteval lookup remains for no-metadata fixtures,
  pending consteval replay by `PendingConstevalExpr::fn_name`, and the
  address-of consteval diagnostic path; rendered enum/const-int maps remain
  structured-first compatibility mirrors when complete value-binding keys are
  unavailable.
- Closure scope: these remaining rendered surfaces are recorded as
  compatibility or follow-up work, not blockers for this source idea. The
  converted route satisfies the requirement to fence one metadata-rich caller
  and prove fail-closed behavior after complete structured miss.

Follow-up recommendation:
- Open a successor idea for carrying structured consteval identity through
  `PendingConstevalExpr` and recursive consteval evaluation. That is the
  highest-value remaining metadata-rich surface after idea 192; broader
  template retry/instantiation carrier migration should stay separate from this
  closure.

Closure readiness:
- Idea 192 is ready for plan-owner close review.

## Suggested Next

Ask the plan owner to review closure for idea 192.

## Watchouts

Do not treat the address-of consteval diagnostic path as a blocker for idea 192
closure; it is diagnostic/no-metadata compatibility, not semantic call-lowering
authority. The follow-up should focus on `PendingConstevalExpr` and recursive
`evaluate_consteval_call` identity, not broad template-engine redesign.

## Proof

No new command was required for this evidence-recording packet. Existing
focused proof remains the relevant route proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_hir_lookup_tests$' > test_after.log`

Result: PASS, `2/2` tests passed. Proof log: `test_after.log`.

Supervisor focused regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS, before `2/2`, after `2/2`, no new failures.

Supervisor full-suite baseline acceptance:
`test_baseline.log` compared with `test_baseline.new.log` for commit
`78f3cc95d`.

Result: PASS, both logs reported `3137/3137`, no new failures. Canonical logs
available for close review: `test_baseline.log`, `test_before.log`, and
`test_after.log`.
