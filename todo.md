Status: Active
Source Idea Path: ideas/open/192_hir_compile_time_rendered_registry_api_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Structured Miss Fail-Closed Behavior

# Current Packet

## Just Finished

Completed Step 3 - Prove Structured Miss Fail-Closed Behavior.

Evidence recorded:
- `test_consteval_call_lowering_rejects_stale_rendered_registry_after_metadata_miss`
  covers the converted direct consteval call-lowering route. The fixture keeps
  the rendered `stale_consteval` registry entry available, corrupts only the
  callee reference TextId to `missing_consteval`, verifies
  `find_consteval_def(call_ref, "stale_consteval") == nullptr`, and then
  asserts direct lowering does not create a pending consteval expression that
  recovered through the stale rendered registry name.
- The route is implemented through
  `Lowerer::try_lower_consteval_call_expr` calling
  `ct_state_->find_consteval_def(n->left, n->left->name)`, so complete
  structured metadata misses fail closed before rendered compatibility can
  participate.

Compatibility boundary:
- `CompileTimeState::find_consteval_def(std::string)` remains available for
  explicit no-metadata compatibility callers.
- Recursive consteval evaluation still receives rendered maps alongside
  TextId/key maps; this packet only converted the direct HIR call-lowering
  route that already has callee metadata.
- No expectation downgrade or testcase-shaped shortcut was used; the proof
  exercises the semantic lookup boundary rather than weakening the call
  lowering contract.

## Suggested Next

Execute Step 4 by auditing the remaining rendered consteval compatibility
surface and deciding whether it is explicitly no-metadata compatibility or
needs a separate conversion/fence packet.

## Watchouts

Do not remove rendered consteval maps blindly; `evaluate_consteval_call` still
receives them as an explicit compatibility surface alongside TextId/key maps.
The address-of consteval diagnostic path in `scalar_control.cpp` still uses the
rendered `has_consteval_def(name)` overload and remains a Step 4 compatibility
classification target.

## Proof

No new command was required for this evidence-only packet. Existing Step 2
proof remains the relevant proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_hir_lookup_tests$' > test_after.log`

Result: PASS, `2/2` tests passed. Proof log: `test_after.log`.

Supervisor focused regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS, before `2/2`, after `2/2`, no new failures.
