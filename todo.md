Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire selected legacy type mirrors only with parity

# Current Packet

## Just Finished

Completed plan.md Step 3 for the selected global aggregate target. The global
printer now renders aggregate global type names from `LirGlobal.llvm_type_ref`
`StructNameId` authority when present, while preserving normal final LLVM
output. Focused coverage proves stale `LirGlobal.llvm_type` and stale
`LirGlobal.llvm_type_ref` text cannot drive aggregate global printer output
when the structured name id remains valid.

## Suggested Next

Execute plan.md Step 4 for the same global aggregate target. Retire only the
exact selected legacy global type mirror if all named verifier/printer/consumer
parity is proven; otherwise record a bounded no-code conclusion that
`LirGlobal.llvm_type` must remain compatibility/output text for now.

## Watchouts

Keep this route to global/extern type facts. Initializer payload semantics,
global policy identity, non-type string routing, collector-only migration, and
Raw-BIR receiver work are separate successor scopes unless an exact selected
typed handoff is accepted.

Extern return migration remains a valid later target but is not first: the
current `LirExternDecl` surface has return text plus a structured return mirror
and a nominal function-signature ref, while fixed parameter type facts are
owned by `function_signature_store` rather than direct extern declaration
fields. A coherent extern-parameter packet likely needs a separate direct
extern signature surface or a deliberately store-backed consumer migration.

Do not widen Step 4 into extern declaration returns, direct extern parameter
surfaces, initializer payload semantics, or backend receiver migration. The
selected carrier is complete only for declared aggregate globals; scalar,
pointer, array, and stale owner-key-miss global rows are compatibility or
later-family work unless a later packet names them explicitly.

## Proof

Step 3 proof command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_global_type_ref$|^frontend_lir_global_label_address_initializer$|^frontend_lir_extern_decl_type_ref$'; } > test_after.log 2>&1`
