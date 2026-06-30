# RV64 ULE Select-Edge Rematerialization Consumer

Status: Closed
Type: RV64 target-consumer idea
Parent: `ideas/closed/464_select_carrier_alias_metadata.md`
Source Evidence: `build/agent_state/464_step4_residual_disposition/`
Owning Layer: RV64 object-route consumer for select-edge ULE source producers

## Goal

Consume `PreparedSelectCarrierAliasAuthorityRecords` to determine whether RV64
can rematerialize `%t46 = bir.ule ptr %t42, %t45` at predecessor edge
`logic.rhs.end.40 -> logic.end.41` for the `%t46 -> %t50` join-transfer
publication, without raw `.phi.sel` alias inference or plain successor-defined
value copying.

## Completion Notes

Closed after the parked prerequisite chain completed. Step 3 implemented and
covered the RV64 consumer for explicit `PreparedSelectCarrierAliasAuthority`
records. Step 4 originally parked this idea because the real representative did
not yet publish a matching available carrier-alias authority record.

Ideas 466 and 468 resolved that prerequisite:

- 466 proved the row was visible but rejected as `unsupported_carrier_alias`;
- 468 implemented shared carrier-alias identity publication before const
  consumers and object-route consumers run;
- the representative `%t46 -> %t50` row is now available with
  `carrier_aliases=2` and `source_use_closure=yes`;
- `20010329-1` prepared and object routes both exit `0`.

No remaining RV64 ULE select-edge consumer packet is known for this chain.

Close validation used existing canonical regression logs and `git diff --check`.

## Reviewer Reject Signals

- Reject any implementation that infers carrier aliases from `.phi.sel` names,
  raw select shape, value ids, block labels, function names, testcase names, or
  dump order instead of consuming `PreparedSelectCarrierAliasAuthorityRecords`.
- Reject a plain `%t46 -> %t50` copy or same-register no-op for the
  successor-defined `%t46`.
- Reject generic move-bundle support claimed as progress for this narrow ULE
  source-producer consumer.
- Reject stale stack-load consumption, expectation rewrites, unsupported-marker
  downgrades, allowlists, pass/fail accounting changes, baseline/log churn, or
  classification-only changes presented as RV64 consumer progress.
