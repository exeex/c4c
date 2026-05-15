Status: Active
Source Idea Path: ideas/open/247_explicit_got_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Hand Back To Address Materialization

# Current Packet

## Just Finished

Step 4 validated the explicit GOT policy prerequisite and recorded the handoff
back to the address-materialization work.

- Fresh backend proof confirms the accepted prerequisite state carries explicit
  GOT policy end to end through target options, BIR symbol metadata, prepared
  address-materialization facts, and AArch64 selected `GotPageLow12` records.
- Prepared/AArch64 selection now depends on structured `GotGlobal` plus
  explicit `GotRequired` policy, symbol identity, result home, and register
  facts. It does not infer GOT from symbol spelling or `is_extern`.
- Terminal GOT and label printing remain intentionally deferred; selected
  `GotPageLow12` and `LabelPageLow12` printer paths fail closed with explicit
  diagnostics until their relocation sequences are specified.
- This completes the policy prerequisite that blocked the earlier address
  materialization route. Idea 233 can resume at its GOT materialization step
  using the prepared/AArch64 selected GOT carrier instead of inventing policy.

## Suggested Next

Resume idea 233 Step 6: populate/continue GOT-backed global materialization from
the explicit prepared/AArch64 selected `GotPageLow12` carrier, keeping terminal
GOT printer output in the idea 233 printer-owned step.

## Watchouts

- This handoff did not implement terminal GOT assembly output.
- Idea 233 printer work should consume `AddressMaterializationRecord` fields and
  reject unsupported GOT/label/TLS cases explicitly rather than formatting from
  names.
- TLS/GOT remains unsupported and fails closed through the prepared/AArch64
  record validation path.
- If the next route needs broader visibility/preemptibility policy, add it by
  populating the explicit BIR policy field instead of reintroducing external
  linkage inference.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed, 139/139 backend tests green. Proof log: `test_after.log`.
