Status: Active
Source Idea Path: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Consumer Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 456. Summary artifact:
`build/agent_state/456_step3_cast_dependency_consumer/summary.md`.

Implemented the narrow RV64 select-edge move materializer route for explicit
prepared dependency-operand authority records with
`policy=rematerialize_cast_from_source` and `status=available`.

The consumer now:

- matches prepared edge publication/source producer facts, predecessor and
  successor labels, source/destination value identity, operand role,
  dependency value identity, cast producer identity, and cast source identity;
- accepts only cast sources with prepared register or rematerializable `i32`
  immediate homes;
- treats the cast producer as edge-owned only when every use of the cast result
  is the authorized source-producer compare operand;
- rejects the LHS-authority/RHS-`t3` scratch-clobber edge where the later RHS
  operand would be read from x28 after LHS materialization overwrote it;
- rematerializes the cast source into a scratch GPR and emits the compare into
  the prepared edge destination register;
- suppresses the owned cast producer instruction so the object route does not
  consume raw `inttoptr` or fall back to stack-slot materialization.

Focused backend coverage proves the accepted explicit cast-dependency route and
fail-closed behavior for missing authority/cast producer, unsupported stack-slot
cast source homes, mismatched dependency operand identity, extra non-carrier
uses of an otherwise-authorized cast result, and the LHS-authority/RHS-`t3`
scratch-clobber overlap.

Representative probe artifacts under
`build/agent_state/456_step3_cast_dependency_consumer/` show `20010329-1`
prepared output still contains the expected `%t17`
`rematerialize_cast_from_source status=available` record. The object probe still
exits 2 with the broad
`unsupported_move_bundle_target_shape: prepared move bundle requires unsupported
RV64 moves` diagnostic, so full representative success is not claimed in this
slice.

## Suggested Next

Step 4: `Residual Disposition And Close Readiness`.

Re-probe and classify the remaining `20010329-1` object-route
`unsupported_move_bundle_target_shape` after the explicit cast-dependency
consumer route. Decide whether the residual belongs to another stack-home,
move-bundle materialization, select-edge, or adjacent owner, and whether idea
456 is close-ready for the accepted consumer route or needs a precise follow-up
packet.

Suggested proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Full `20010329-1` object success remains unclaimed; Step 3 only proves the
  explicit cast-dependency consumer route in focused coverage.
- Continue to consume only populated `rematerialize_cast_from_source
  status=available` dependency-operand authority.
- Preserve the scratch-clobber guard unless a later packet implements a
  register-safe materialization ordering or scratch allocator.
- Do not infer from raw `inttoptr`, stack homes, object metadata, filenames,
  function names, block names, or one prepared dump.
- Do not consume `load_from_stack_slot` while the row is
  `missing_stack_freshness`.
- Do not copy successor/join-block compare results on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, baseline logs, or `review/`.

## Proof

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed; proof output is in `test_after.log`.
