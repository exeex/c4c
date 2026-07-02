Status: Active
Source Idea Path: ideas/open/554_out_of_ssa_parallel_copy_move_bundle_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Map Producer Facts

# Current Packet

## Just Finished

Lifecycle transition completed. The evidence-gap idea was closed and this
follow-up producer/fact-propagation route was activated.

## Suggested Next

Execute Step 1 from `plan.md`.

## Watchouts

- Do not infer missing move bundles in the prepared consumer.
- Do not special-case `src/960209-1.c`, block label 20, predecessor 20,
  successor 19, or block index 15.
- Preserve the structured missing-bundle diagnostic while repairing producer
  publication facts.

## Proof

Close-gate baseline and after logs were captured with:

```sh
ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_object_consumer_contract|backend_riscv_object_emission)$'
```

Regression guard passed with `2/2` tests passing before and after. Canonical
logs are `test_before.log` and `test_after.log`.
