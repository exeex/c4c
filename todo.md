Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prefer Structured Lookup For Constant Initializers

# Current Packet

## Just Finished

Completed `plan.md` Step 5 constant-initializer aggregate lookup migration.
Struct and nested union byte-encoding initializer paths now call
`lookup_structured_layout` through a `ConstInitEmitter` helper when resolving
known struct/union `TypeSpec`s, while keeping the legacy `HirStructDef`
authoritative for field order, default values, and rendered initializer text.

## Suggested Next

Supervisor can review remaining constant-initializer aggregate callers, if any,
or move Step 5 toward acceptance review.

## Watchouts

The migrated initializer helper intentionally returns `layout.legacy_decl`, so
structured lookup is attempted when the `StructNameId` is known but initializer
semantics still follow `TypeSpec::tag` / `mod.struct_defs`. This packet did not
change emitted LLVM text or test expectations.

## Proof

Delegated proof passed and wrote `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|verify_tests_verify_empty_initializer|positive_sema_ok_expr_access_misc_runtime_c|llvm_gcc_c_torture_src_(struct_ini_[0-9]_c|strct_.*_c|zero_struct_[12]_c))'; } > test_after.log 2>&1`.

The selected subset passed: 18/18 tests.
