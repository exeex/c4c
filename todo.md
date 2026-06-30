Status: Active
Source Idea Path: ideas/open/455_dependency_operand_authority_population.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route First Population Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 455. Implemented producer/prepared population and
printer exposure for explicit dependency-operand authority records.

What changed:

- added `collect_prepared_dependency_operand_authorities` as a prepared
  metadata view over edge publications, source-producer facts, dependency
  operand homes, stack-object linkage, cast producers, and cast-source homes;
- added a prepared printer section
  `--- prepared-dependency-operand-authorities ---`;
- populated the supported `%t17`-class shape as
  `policy=rematerialize_cast_from_source status=available` only when the
  source producer is a prepared binary compare, the dependency operand is a
  named pointer with a stack-slot home, the dependency has an explicit prepared
  cast producer, and the cast source has a consumable home;
- emitted the adjacent stack-load policy as fail-closed
  `policy=load_from_stack_slot status=missing_stack_freshness`;
- added focused prepared-printer coverage for the available cast
  rematerialization record and fail-closed stack-load record;
- left RV64 target lowering untouched.

Representative evidence:

- `build/agent_state/455_step3_dependency_operand_population/20010329-1.prepared.out`
  prints the `logic.rhs.end.13 -> logic.end.14` `%t18 -> %t22` dependency
  operand `%t17` as `rematerialize_cast_from_source` with cast source `%t16`
  home `rematerializable_immediate` and immediate `-2147483643`;
- the same prepared dump prints the parallel `load_from_stack_slot` authority
  row with `status=missing_stack_freshness`.

Summary artifact:
`build/agent_state/455_step3_dependency_operand_population/summary.md`.

## Suggested Next

Step 4: `Residual Disposition And Close Readiness`.

Re-probe or inspect the `20010329-1` select-edge dependency residuals against
the new printed dependency-operand authority section. Decide whether idea 455 is
complete as population/printing work, should route back to the stack-slot
pointer select-edge consumer only after supervisor selection, or needs a
separate follow-up for stack-load freshness/clobber facts.

Proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- The new authority records are producer/prepared facts only; no RV64 consumer
  lowering was added in this packet.
- `load_from_stack_slot` remains unavailable until a separate producer owns
  freshness and clobber-safety.
- Successor-result copies remain rejected; the available path rematerializes
  the dependency operand's cast from its source home.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_baseline.log`, `test_baseline.new.log`,
  or `review/`.

## Proof

Step 3 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed. Proof log: `test_after.log`.
