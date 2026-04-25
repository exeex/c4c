# Review: 110 LIR Struct Type Printer Authority Readiness Audit

## Scope

This audit covers the rendered struct/type string surfaces named by
`ideas/open/110_lir_struct_type_printer_authority_readiness_audit.md`:
`LirModule::type_decls`, `LirModule::struct_decls`, `LirTypeRef` struct-name
mirrors, global/function/extern signature text fields, HIR-to-LIR structured
layout lookup observations, LIR verifier parity checks, and the focused Step 3
proof recorded in `todo.md`.

No legacy removal was performed. No implementation files, tests, expectations,
`plan.md`, or source ideas were changed by this audit.

## Classification

| Surface | Classification | Evidence |
| --- | --- | --- |
| `LirModule::struct_decls` plus `render_struct_decl_llvm()` | `printer-authority-ready` for struct declaration lines only | `src/codegen/lir/ir.hpp:676` stores the structured declaration mirror beside legacy `type_decls`; `src/codegen/lir/lir_printer.cpp:503` renders a structured declaration body; `src/codegen/lir/verify.cpp:409` through `src/codegen/lir/verify.cpp:441` requires each structured declaration to resolve a name, validate field type refs, and shadow-render exactly against the legacy line. |
| `LirModule::type_decls` | `legacy-proof-only` for declaration parity, but still `bridge-required` for backend/MIR layout consumers | The LIR printer still emits `type_decls` at `src/codegen/lir/lir_printer.cpp:529` through `src/codegen/lir/lir_printer.cpp:530`. BIR and MIR backend code still consume `module.type_decls` as the aggregate layout/type body source, including `src/backend/mir/aarch64/codegen/emit.cpp` and `src/backend/bir/lir_to_bir/memory/*`. |
| `LirTypeRef` struct-name mirrors | `blocked-by-type-ref` for broad authority; ready only as parity mirrors on selected top-level surfaces | `src/codegen/lir/types.hpp:54` through `src/codegen/lir/types.hpp:83` carries `StructNameId`, but equality and output still use text. The Step 3 frontend mirror tests prove selected globals, signatures, extern returns, and direct call refs, not all type refs. |
| Global type text: `LirGlobal::llvm_type` | `printer-only` plus `legacy-proof-only` where `llvm_type_ref` exists | `src/codegen/lir/ir.hpp:540` through `src/codegen/lir/ir.hpp:542` keeps `llvm_type` as emitted text with an optional mirror. `src/codegen/lir/lir_printer.cpp:543` through `src/codegen/lir/lir_printer.cpp:551` prints `g.llvm_type`; `src/codegen/lir/verify.cpp:467` through `src/codegen/lir/verify.cpp:480` only checks mirror parity. `tests/frontend/frontend_lir_global_type_ref_test.cpp:109` through `tests/frontend/frontend_lir_global_type_ref_test.cpp:113` asserts the printer still uses the legacy text. |
| Function signature text: `LirFunction::signature_text` | `printer-only` with selected `legacy-proof-only` mirrors | `src/codegen/lir/ir.hpp:506` through `src/codegen/lir/ir.hpp:514` stores signature mirrors beside the preformatted signature. `src/codegen/lir/verify.cpp:575` through `src/codegen/lir/verify.cpp:621` compares mirrors against parsed signature text. `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp:158` through `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp:170` asserts the printer still uses `signature_text`. |
| Extern return type text: `LirExternDecl::return_type_str` | `printer-only` with `legacy-proof-only` mirror checks | `src/codegen/lir/ir.hpp:489` through `src/codegen/lir/ir.hpp:493` stores both text and `LirTypeRef`. `src/codegen/lir/verify.cpp:445` through `src/codegen/lir/verify.cpp:464` checks exact return mirror parity and rejects known struct returns without the matching `StructNameId`. `tests/frontend/frontend_lir_extern_decl_type_ref_test.cpp:76` through `tests/frontend/frontend_lir_extern_decl_type_ref_test.cpp:80` verifies emission still uses return text. |
| Call return/argument type refs | `blocked-by-type-ref` for broad authority; selected direct-call mirrors are `legacy-proof-only` | `tests/frontend/frontend_lir_call_type_ref_test.cpp:121` through `tests/frontend/frontend_lir_call_type_ref_test.cpp:137` proves direct struct call mirrors and existing formatted call text. `tests/frontend/frontend_lir_call_type_ref_test.cpp:139` through `tests/frontend/frontend_lir_call_type_ref_test.cpp:145` records that variadic aggregate calls do not carry argument mirrors when ABI-emitted arguments cannot be parsed into the source signature. |
| HIR-to-LIR layout lookup by `StructNameId` | `blocked-by-layout-lookup` for authority; useful as parity observation | `src/codegen/lir/hir_to_lir/core.cpp:116` through `src/codegen/lir/hir_to_lir/core.cpp:135` attempts structured lookup, records parity, and still keeps legacy layout facts. Current consumers such as `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:94` through `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:101`, `src/codegen/lir/hir_to_lir/call/args.cpp:166` through `src/codegen/lir/hir_to_lir/call/args.cpp:170`, and `src/codegen/lir/hir_to_lir/call/vaarg.cpp:148` through `src/codegen/lir/hir_to_lir/call/vaarg.cpp:152` still consume `legacy_decl`. |
| LIR verifier structured layout observations | `legacy-proof-only` | `src/codegen/lir/verify.cpp:635` through `src/codegen/lir/verify.cpp:670` rejects malformed observations and parity mismatches, but it is a proof hook rather than emitted-output authority. |

## Proof Considered

Step 3 in `todo.md` records the focused proof:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_.*|llvm_gcc_c_torture_src_(align_[123]_c|struct_ini_[0-9]_c|struct_ret_[12]_c|strct_.*_c|zero_struct_[12]_c))'; } > test_after.log 2>&1
```

The recorded result was a successful no-op build plus 25/25 passing matching
tests. That proof covers the frontend LIR mirror tests for globals, function
signatures, extern returns, and calls, plus focused ABI/variadic/layout/runtime
coverage for aggregate paths. No new tests were run for this report-only
packet.

## Blockers

The printer-authority switch should not include globals, function signatures,
extern declarations, call sites, or backend layout consumers yet. Those
surfaces still emit or consume legacy text by design, and the available mirrors
are parity checks rather than complete structured-authority replacements.

The remaining concrete blockers are:

- Broad type-ref authority is blocked by incomplete `LirTypeRef` coverage for
  ABI-shaped parameter fragments, variadic aggregate call arguments, and other
  raw-text type refs. The frontend mirror tests prove selected direct cases,
  not universal type-ref replacement.
- Layout authority is blocked by HIR-to-LIR consumers that still take
  `StructuredLayoutLookup::legacy_decl` after observing structured parity.
  This is visible in const initialization, variadic aggregate argument
  handling, and `va_arg` aggregate handling.
- Backend/MIR aggregate layout still depends on `module.type_decls`, so
  removing or globally demoting `type_decls` would cross out of printer-only
  declaration authority and into backend layout authority.

## Recommendation

Create a follow-up implementation idea that switches only LLVM struct
declaration printing from `type_decls` to `struct_decls`, with `type_decls`
retained as a verifier/backcompat proof source. That idea should keep
`verify_struct_decl_shadows()` until a later removal audit proves the legacy
line can be deleted.

Do not combine that declaration-printer switch with broader demotion of
global/function/extern/call type text or backend layout use of `type_decls`.
Those surfaces need more dual-path coverage and authority-specific follow-up
work before they are safe to treat as structured output authority.
