Status: Active
Source Idea Path: ideas/open/192_hir_compile_time_rendered_registry_api_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Convert Or Fence One Metadata-Rich Route

# Current Packet

## Just Finished

Completed Step 2 - Convert Or Fence One Metadata-Rich Route.

Implementation summary:
- `Lowerer::try_lower_consteval_call_expr` now calls
  `ct_state_->find_consteval_def(n->left, n->left->name)`, so direct
  consteval call lowering uses the callee `Node*` metadata before considering
  rendered spelling compatibility.
- Added
  `test_consteval_call_lowering_rejects_stale_rendered_registry_after_metadata_miss`
  in `tests/frontend/frontend_hir_lookup_tests.cpp`. The fixture keeps a
  rendered `stale_consteval` registry entry available, corrupts only the call
  reference TextId to a complete structured miss, and proves the lowering route
  does not recover through the stale rendered registry name.

Compatibility boundary:
- `CompileTimeState::find_consteval_def(std::string)` remains available for
  explicit no-metadata compatibility callers.
- Recursive consteval evaluation still receives rendered maps alongside
  TextId/key maps; this packet only converted the direct HIR call-lowering
  route that already has callee metadata.

## Suggested Next

Execute Step 3 by recording/proving structured miss fail-closed behavior for
the converted direct consteval call-lowering route. The focused test added in
this packet already covers stale rendered fallback rejection for that route.

## Watchouts

Do not remove rendered consteval maps; `evaluate_consteval_call` still receives
them as an explicit compatibility surface alongside TextId/key maps. The
address-of consteval diagnostic path in `scalar_control.cpp` still uses the
rendered `has_consteval_def(name)` overload and remains a follow-up candidate,
but it was outside this packet.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_hir_lookup_tests$' > test_after.log`

Result: PASS, `2/2` tests passed. Proof log: `test_after.log`.

Supervisor focused regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS, before `2/2`, after `2/2`, no new failures.
