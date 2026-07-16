Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add or complete the selected extern family-ref carrier

# Current Packet

## Just Finished

Completed repaired plan.md Step 1 inventory. Extern return facts currently flow
from `record_extern_decl` through `ExternDeclInfo.return_type_str` and
`ExternDeclInfo.return_type`, then finalize into
`LirExternDecl.return_type_str` and `LirExternDecl.return_type`; verifier parity
is `verify_extern_decl_shadows`, printer output still reads
`return_type_str`, and aggregate return identity can carry `StructNameId`.
Extern fixed parameter facts are not direct `LirExternDecl` fields; they live
in `LirFunctionSignatureStoreEntry.fixed_param_type_refs` behind
`function_signature_ref`.

Selected repaired Step 2 target: complete the bounded extern aggregate return
carrier for `LirExternDecl.return_type` first. Accepted authority input is the
module-owned extern return `LirTypeRef` with `StructNameId` produced from the
struct-name/declaration store. Compatibility mirror is
`LirExternDecl.return_type_str`, which must remain final declaration output text
until selected printer parity is proven. Rejected authority inputs are extern
raw names, `LinkNameId` symbol identity, final declaration text alone,
signature-store parameter facts, global initializer text, and collector/import
receiver observations.

## Suggested Next

Execute repaired plan.md Step 2 for the selected extern aggregate return
target. Require/publish a structured `LirExternDecl.return_type` carrier for
known aggregate returns while preserving `return_type_str` as compatibility
output text.

## Watchouts

Keep this repaired route inside idea 844 global/extern type facts. Do not absorb
initializer payload semantics, global policy identity, collector-only migration,
Raw-BIR receiver work, or non-type string routing. Extern parameter/signature
store facts remain a later target because `LirExternDecl` has no direct
parameter carrier today.

Step 2 proof needs: positive struct/union extern returns, missing carrier for a
known aggregate return, wrong `StructNameId`, stale `return_type_str` text that
must not drive authority after the structured carrier is selected, nonaggregate
return compatibility, and unchanged global initializer compatibility.

## Proof

Inventory proof command: `git diff --check`. Suggested focused code-packet
proof: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_extern_decl_type_ref$|^frontend_lir_global_label_address_initializer$'; } > test_after.log 2>&1`
