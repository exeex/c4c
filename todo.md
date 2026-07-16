Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire selected extern legacy mirrors only with parity

# Current Packet

## Just Finished

Completed repaired plan.md Step 3 for the selected extern aggregate return
target. The extern declaration printer now renders aggregate return type names
from `LirExternDecl.return_type` `StructNameId` authority when present, and the
verifier treats the structured carrier as selected authority even when retained
`return_type_str` or return mirror text is stale. Focused coverage proves stale
extern return text cannot drive selected aggregate declaration output.

## Suggested Next

Execute repaired plan.md Step 4 for the selected extern aggregate return
target. Retire only the exact selected extern return text mirror if all named
verifier/printer/consumer parity is proven; otherwise record a bounded no-code
conclusion that `return_type_str` must remain compatibility/output text for now.

## Watchouts

Keep this repaired route inside idea 844 global/extern type facts. Do not absorb
initializer payload semantics, global policy identity, collector-only migration,
Raw-BIR receiver work, or non-type string routing. Extern parameter/signature
store facts remain a later target because `LirExternDecl` has no direct
parameter carrier today.

Do not widen Step 4 into extern parameter/signature-store migration,
collector/import-preparation, Raw-BIR receiver work, or nonaggregate runtime
text handling.

## Proof

Step 3 proof command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_extern_decl_type_ref$|^frontend_lir_global_label_address_initializer$'; } > test_after.log 2>&1`
