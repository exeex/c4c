Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate selected extern verifier/printer consumers

# Current Packet

## Just Finished

Completed repaired plan.md Step 2 for the selected extern aggregate return
target. The existing producer path already publishes `LirExternDecl.return_type`
with `StructNameId` for known aggregate returns and verifier coverage already
rejected missing structured carriers; this packet added focused wrong-authority
coverage for a return carrier with the wrong `StructNameId`.

## Suggested Next

Execute repaired plan.md Step 3 for the selected extern aggregate return
target. Move extern declaration printer/verifier behavior to treat
`LirExternDecl.return_type` `StructNameId` as semantic authority while
preserving normal declaration output compatibility.

## Watchouts

Keep this repaired route inside idea 844 global/extern type facts. Do not absorb
initializer payload semantics, global policy identity, collector-only migration,
Raw-BIR receiver work, or non-type string routing. Extern parameter/signature
store facts remain a later target because `LirExternDecl` has no direct
parameter carrier today.

Do not widen Step 3 into extern parameter/signature-store migration. Stale
`return_type_str` should become compatibility text for selected aggregate
returns only when the structured carrier remains valid; nonaggregate returns
remain runtime-text compatibility.

## Proof

Step 2 proof command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_extern_decl_type_ref$|^frontend_lir_global_label_address_initializer$'; } > test_after.log 2>&1`
