Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate selected verifier, printer, and collection consumers

# Current Packet

## Just Finished

Completed plan.md Step 2 for the selected global aggregate target. Declared
aggregate global types must now carry `LirGlobal.llvm_type_ref` with matching
`StructNameId`; absent refs still remain allowed for stale owner-key misses and
unselected scalar/pointer/array globals. Added focused coverage proving a
declared aggregate global without the carrier is rejected while preserving the
existing stale compatibility text, wrong `StructNameId`, and initializer output
coverage.

## Suggested Next

Execute plan.md Step 3 for the same global aggregate target. Migrate one
selected consumer that still treats `LirGlobal.llvm_type` as semantic input to
prefer `LirGlobal.llvm_type_ref`, while preserving final LLVM output and
initializer scanner compatibility.

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

Do not widen Step 3 into extern declaration returns or direct extern parameter
surfaces. The selected carrier is complete only for declared aggregate globals;
scalar, pointer, array, and stale owner-key-miss global rows are compatibility
or later-family work unless a later packet names them explicitly.

## Proof

Step 2 proof command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_global_type_ref$|^frontend_lir_global_label_address_initializer$|^frontend_lir_extern_decl_type_ref$'; } > test_after.log 2>&1`
