Status: Active
Source Idea Path: ideas/open/465_rv64_ule_select_edge_rematerialization_consumer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 residual disposition for idea 465. The Step 3 RV64 consumer
is implemented and covered for the explicit carrier-alias authority fixture,
but the fresh `20010329-1` representative probe did not advance: object route
still rejects the same coordinate-bearing `logic.rhs.end.40 -> logic.end.41`
preterminator copy. Idea 465 is not close-ready as representative progress.

Residual table:

| Question | Evidence | Disposition |
| --- | --- | --- |
| Step 3 consumer coverage | `build/agent_state/465_step3_ule_rematerialization_consumer/summary.md` records accepted/fail-closed backend coverage. | Consumer slice is locally valid for explicit authority. |
| Fresh prepared route | `20010329-1.prepared.status=0`; prepared dump still shows `%t46 = bir.ule ptr %t42, %t45`, duplicate `%t50.phi.sel0/%t50.phi.sel1`, `%t46 -> %t50`, and `%t42/%t45` GPR homes. | Representative has the structural inputs. |
| Fresh object route | `20010329-1.object.status=2`; diagnostic remains `unsupported_move_bundle_target_shape ... event_kind=pre_terminator_copies ... logic.rhs.end.40 -> logic.end.41 ... from_value_id=20 to_value_id=21`. | Representative did not advance past the first failing move-bundle event. |
| Carrier-alias evidence in dump | No printed `carrier_alias` / `select_carrier` record appears in the fresh prepared dump. | Next first-owner is verifying/populating/exposing carrier-alias authority for the real representative, not generic RV64 move support. |
| Operand boundary | `%t42` home `s1`, `%t45` home `s2` remain GPR-consumable. | Operand consumability is still not the first blocker. |

Artifacts:
`build/agent_state/465_step4_residual_disposition/`.

## Suggested Next

Do not close idea 465 as representative-complete. The next coherent packet
should first prove why `collect_prepared_select_carrier_alias_authorities` does
not produce a matching authority record for current `20010329-1`, or add
focused producer/printer evidence if the record exists but is not visible.
Only after that should RV64 consumer behavior be revisited.

## Watchouts

- Do not broaden any follow-up into generic move-bundle lowering, plain
  successor-defined copies, or same-register no-ops.
- Do not infer duplicate-carrier aliases from `%*.phi.sel*` spelling, raw
  select shape, value ids, block labels, function names, testcase names, or
  dump order.
- Do not treat the Step 3 fixture success as representative close-readiness
  until `20010329-1` publishes and consumes a matching carrier-alias authority
  record.
- Preserve record-based fail-closed behavior for missing/wrong alias authority,
  wrong edge, wrong source producer, mismatched carrier/final result, and
  non-consumable operands.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 4 proof:

```sh
git diff --check
```

Result: passed.
