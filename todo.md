Status: Active
Source Idea Path: ideas/open/487_bir_dynamic_local_array_proof_source_path_no_clobber_population.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Real Proof Population Contract

# Current Packet

## Just Finished

Completed Step 2 contract/routing classification for idea 487.

Decision:

| Contract question | Step 2 answer |
| --- | --- |
| Can a real proof-source/path/no-clobber population contract be implemented now? | No. The checker contract exists, and branch/control-flow facts exist, but dynamic local-array paths still lack a durable consumer coordinate and prepared exposure key. |
| First lower-level contract required | Dynamic local-array element-path consumer-coordinate / prepared-exposure carrier. |
| Required carrier fields | Shared function identity, element-path identity, dynamic index value, source object, derivation result, layout/range, consumer block label, consumer instruction index, consumer operation role, and stable prepared lookup key. |
| Later proof population requirements | Same-function proof source, normalized lower/upper predicates and bounds, operand-role match to the dynamic index, path/dominance coverage, and no-clobber/same-value interval facts. |
| Fail-closed statuses | Missing proof source/bounds/path/no-clobber, unsupported source/predicate/index width, operand-role or bound mismatch, non-dominating/uncovered path, clobbered or redefined index, effect-unknown intervals, raw-shape-only, target-only/final-home-only, and protected boundaries. |
| Step 3 readiness | Not ready for real proof population. Step 3 should route or split to the consumer-coordinate/prepared-exposure carrier prerequisite. |

Supporting artifacts:

- `build/agent_state/487_step1_real_proof_population_inputs/audit.md`
- `build/agent_state/487_step2_real_proof_population_contract/contract.md`

## Suggested Next

Execute Step 3 as a routing packet: record that real proof-source/path/no-clobber
population is blocked until the lower-level dynamic local-array
consumer-coordinate / prepared-exposure carrier exists, or split/activate that
carrier owner if lifecycle policy requires it.

## Watchouts

- Do not infer range proof from loop shape, variable names, testcase names,
  dump order, final homes, or RV64 target behavior.
- Do not mark dynamic rows available without real proof-source,
  path/dominance, and no-clobber facts accepted by the idea 486 checker.
- Do not change idea 486 checker/status vocabulary, idea 484 packaging, scalar
  local-load consumption, or RV64/MIR lowering in this population packet.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.
- Do not attempt branch proof-source matching by result names, loop shape, dump
  order, or implicit proximity between a branch and a dynamic GEP/access.
- Keep idea 486 checker/status vocabulary unchanged; the missing work is real
  producer evidence, not another synthetic checker case.

## Proof

Step 2 validation:

```sh
git diff --check
```

Result: passed.
