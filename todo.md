Status: Active
Source Idea Path: ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize The Move-Bundle Authority

# Current Packet

## Just Finished

Activated Step 1 for `ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md`
after closing the completed 574 FP binary lowering idea.

## Suggested Next

Delegate Step 1 to reproduce and localize the prepared
`ambiguous_non_parallel_multi_source_stack_destination` blocker from the 574
Step 4c representative artifact without changing implementation code.

## Watchouts

- The 574 FP binary acceptance criteria are complete; do not fold this
  move-bundle authority work back into FP binary lowering.
- Do not bypass the prepared move-bundle classifier broadly. Step 1 must first
  classify whether the source/destination authority is ordered, mutually
  exclusive, or genuinely ambiguous.
- Keep filename/function/value-name matching out of the route.

## Proof

Activation-only lifecycle update. Close-time guard for 574 regenerated matching
backend logs:

```sh
cmake --build --preset default &&
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: 346/346 backend tests passed before and after. Regression guard passed
with `--allow-non-decreasing-passed`.
