Status: Active
Source Idea Path: ideas/open/454_edge_dependency_operand_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Producer Metadata Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 454. Summary artifact:
`build/agent_state/454_step3_dependency_operand_authority_metadata/summary.md`.

Implemented metadata-only surface:

- `PreparedDependencyOperandMaterializationPolicy`
- `PreparedDependencyOperandAuthorityStatus`
- `PreparedDependencyOperandAuthorityInputs`
- `PreparedDependencyOperandAuthority`
- `plan_prepared_dependency_operand_authority`
- `prepared_dependency_operand_authority_available`

Accepted authority:

- edge publication must be available, block-entry,
  predecessor-terminator placed, register-destination, and backed by a binary
  source producer;
- dependency operand must match the selected producer operand role;
- dependency value home must match the operand value id/name/type;
- stack-backed operands must have coherent stack object linkage;
- `rematerialize_cast_from_source` requires explicit cast producer, supported
  `IntToPtr` width, matching cast result, and register or rematerializable
  immediate cast-source home;
- `load_from_stack_slot` requires explicit freshness and clobber-safety.

Focused tests cover accepted explicit cast-rematerialization, accepted explicit
stack-load only with freshness/clobber-safety, and fail-closed missing policy,
missing cast source, bad cast source home, unsupported cast width, missing
freshness, missing clobber-safety, home mismatch, operand mismatch, and
unavailable edge publication.

No RV64 target lowering was changed or allowed to consume the new authority in
this packet.

## Suggested Next

Step 4: `Residual Disposition And Close Readiness`.

Re-check the `%t17` dependency-operand authority status against the new
metadata surface. Classify whether idea 454 is complete as a producer/prepared
authority prerequisite, whether a separate population/printing packet remains,
or whether lifecycle should route back to the stack-slot pointer select-edge
consumer only after a plan-owner decision. Do not select RV64 target lowering
inside this packet.

Future proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not route to RV64 target lowering from current facts.
- Do not treat `%t17` stack home plus object metadata as sufficient
  `load_from_stack_slot` authority.
- Do not treat raw `inttoptr` plus `%t16` immediate as sufficient
  `rematerialize_cast_from_source` authority.
- Do not copy `%t18` from the successor/join block on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 3 backend proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
