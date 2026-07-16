Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire selected legacy type mirrors only with parity

# Current Packet

## Just Finished

Completed plan.md Step 4 for the selected global aggregate target as a bounded
no-code retirement conclusion. No safe deletion remains inside the selected
global aggregate slice: `LirGlobal.llvm_type` is no longer semantic authority
for declared aggregate global verifier/printer behavior when a valid
`llvm_type_ref` exists, but it still remains compatibility/output text and a
fallback for unselected scalar, pointer, array, stale owner-key-miss, and
receiver/backend paths.

## Suggested Next

Send the exhausted runbook to plan-owner for semantic disposition of idea 844:
close the bounded selected global aggregate slice, repair the current route
for another in-scope global/extern target, or switch to a separately scoped
successor if remaining durable intent should continue elsewhere.

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

Potential later targets remain extern return facts, direct extern parameter
surface design, and collector/import-preparation migration, but those were not
selected by this runbook route and should be decided by lifecycle disposition
rather than silently absorbed after Step 4.

## Proof

Step 4 proof command: `git diff --check`. The last code proof for this selected
global aggregate route remains `test_after.log` from Step 3:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_global_type_ref$|^frontend_lir_global_label_address_initializer$|^frontend_lir_extern_decl_type_ref$'; } > test_after.log 2>&1`
