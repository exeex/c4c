Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add or complete the selected family-ref carrier

# Current Packet

## Just Finished

Completed plan.md Step 1 inventory for idea 844. Global type facts currently
lower from `hir::GlobalVar.type.spec` into `LirGlobal.type`,
`LirGlobal.llvm_type`, and the optional aggregate-only
`LirGlobal.llvm_type_ref`; verifier parity is
`verify_global_type_ref_shadows`, printer output still reads `llvm_type`, and
initializer reachability uses `initializer_function_link_name_ids` with
`init_text` as a legacy fallback. Extern declaration return facts flow through
`record_extern_decl` / `ExternDeclInfo.return_type_str` /
`ExternDeclInfo.return_type`, then finalize into `LirExternDecl.return_type_str`
and `LirExternDecl.return_type`; verifier parity is
`verify_extern_decl_shadows`, printer output still reads `return_type_str`, and
extern parameter facts live in the module function-signature store rather than
as direct `LirExternDecl` fields.

Selected Step 2 target: complete the bounded global aggregate type carrier for
`LirGlobal.llvm_type_ref` first. The accepted authority input is the
producer-owned global `TypeSpec` plus structured aggregate identity from the
module struct-name/declaration store. The compatibility mirror is
`LirGlobal.llvm_type`, and it must remain final-rendering text until selected
printer/consumer migration proves parity. Rejected authority inputs are
`LirGlobal.init_text`, initializer payload spelling, `qualifier`,
`linkage_vis`, `is_const`, `is_internal`, `is_extern_decl`, raw global names,
extern `return_type_str`, and function-signature/extern parameter facts.

## Suggested Next

Execute plan.md Step 2 for the selected global aggregate target. Add or
complete the minimal family-ref carrier/API around `LirGlobal.llvm_type_ref`
without moving extern declarations, initializer payload semantics, global
policy identity, or collector-only receiver work. Focus coverage on positive
struct/union globals plus missing carrier, stale `llvm_type` text, wrong
StructNameId, missing struct declaration, pointer/array/scalar wrong-family
exclusions, and preservation of initializer compatibility.

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

Missing evidence for Step 2: whether the selected global carrier should remain
optional for scalar/pointer/array globals in this slice, or whether Step 2
should introduce a named global family-ref wrapper while preserving the
existing optional aggregate carrier as its compatibility implementation.

## Proof

Inventory proof command: `git diff --check`. Suggested focused code-packet
proof: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_global_type_ref$|^frontend_lir_global_label_address_initializer$|^frontend_lir_extern_decl_type_ref$'; } > test_after.log 2>&1`
