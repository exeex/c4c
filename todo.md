Status: Active
Source Idea Path: ideas/open/858_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and publish one next body-parameter authority row
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Lifecycle switch from 734 after accepted Step 7.46. The 734 source completion
gate remains unmet, and no further Raw-BIR receiver row is authorized without
a separately scoped producer handoff.

## Suggested Next

Execute `plan.md` Step 1 for `ideas/open/858_lir_next_body_parameter_authority_handoff.md`:
select and publish exactly one next valid function-body parameter-use authority
row, then hand it back to 734 for a later receiver packet.

## Watchouts

Do not edit Raw-BIR/importer receiver code in this successor. Do not repeat or
reopen accepted 734 body-parameter receiver rows through Step 7.46. Do not
select from presentation fields, generic parameter sweeps, ABI-expanded or
aggregate parameter families, memory/VA, aggregate/vector,
module/type/global/metadata, residual instruction/terminator, inline-assembly,
or any other family.

## Proof

Required proof for the successor implementation:

```
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '<focused producer/verifier executable selected by supervisor>'
git diff --check
```

Escalate to a matching broader before/after guard if shared LIR producer or
verifier code is touched.
