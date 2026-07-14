# Current Packet

Status: Active
Source Idea Path: ideas/open/760_lir_string_constructor_deprecation_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add a narrow warning and explicit runtime-text boundary

## Just Finished

- Plan Step 1 complete: AST/header and targeted call-site inventory classifies
  `build_type_decls`' va_list field literals (`"i32"`, `"ptr"`) as the first
  coherent closed-set migration group. The same function's generated array
  storage and padding spellings (`"[N x i8]"`/`pad_ty`) are the retained
  dynamic runtime-text boundary. HIR `llvm_ty`/`ret_ty` lowering, parsed call
  arguments, aggregate names, and verifier text-mirror checks remain deferred
  dynamic/typed-lowering boundaries; opcode/predicate refs already use enum
  constructors where closed identity is directly available.

## Suggested Next

- Step 2 only: add an explicit `LirTypeRef` runtime-text boundary and a narrow
  `const char*` legacy-construction warning, then route only
  `build_type_decls`' generated array/padding text through that boundary.

## Watchouts

- Do not remove or globally silence string construction warnings.
- Do not begin a repository-wide migration or rewrite HIR `TypeSpec` lowering.
- Keep dynamic aggregate, vector, struct, and function text paths supported.
- Do not weaken verifier behavior, tests, or expected output.
- Warning/proof strategy: use a `[[deprecated]]` warning only on the literal
  `const char*` construction surface (not the `std::string` runtime path),
  then build the affected target and run the focused LIR call-type test while
  confirming only the selected va_list literals are migrated.

## Proof

- Read-only inventory only: no build/test proof was required or run, and no
  root-level test log was created or modified. AST queries used
  `c4c-clang-tool`/`c4c-clang-tool-ccdb`, supplemented by targeted `rg`.
