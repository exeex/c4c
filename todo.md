Status: Active
Source Idea Path: ideas/open/445_closed_world_no_external_caller_metadata_source.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Possible Metadata Sources

# Current Packet

## Just Finished

Activated `ideas/open/445_closed_world_no_external_caller_metadata_source.md`
as the next active plan. Selection rationale: it is the direct follow-up from
closed idea 444 and owns the missing source-of-truth problem before
`NoExternalCaller` can be populated.

## Suggested Next

Execute Step 1: audit frontend, HIR, LIR, module, linker/visibility, callgraph,
and whole-program mode surfaces and classify whether any can soundly provide
closed-world/no-external-caller authority.

## Watchouts

- Do not infer authority from observed same-module direct calls, visibility
  spelling, `LinkNameId`, `can_elide_if_unreferenced`, absent extern
  declarations, or testcase shape.
- Account for function-address escape and indirect-call target exclusion before
  accepting any metadata source.
- Keep `930930-1::f` fail-closed unless a real source proves no outside caller
  can provide different pointer values.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
