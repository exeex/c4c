Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce A Shared Structured Layout Lookup Helper

# Current Packet

## Just Finished

Completed `plan.md` Step 2 helper-only packet. Added
`stmt_emitter_detail::lookup_structured_layout`, which accepts the HIR module,
optional `LirModule`, and aggregate `TypeSpec`; preserves the legacy
`mod.struct_defs` lookup result; attempts `%struct.<tag>` to `StructNameId` to
`LirModule::find_struct_decl` when a structured module is available; and
records whether the structured field-type mirror matches the legacy LLVM
layout reconstruction. Corrected the initial staging-only wiring by adding the
same module-aware `object_align_bytes` overload to the live compiled
`hir_to_lir.cpp` monolith and routing `populate_signature_type_refs` through it
with the active `LirModule*`. The legacy no-module overload still delegates to
the same behavior with a null module pointer, so existing callers keep the
legacy `HirStructDef::align_bytes` result or the existing 8-byte miss fallback.

## Suggested Next

Next packet can expand the live size/alignment coverage beyond signature type
refs, preferably by threading `LirModule*` into another low-risk layout path
instead of adding another duplicate helper.

## Watchouts

Both `core.cpp` and `hir_to_lir.cpp` are in `build/compile_commands.json`.
`core.cpp` is compiled, but it still carries the staging comment and is not
the only live implementation of `object_align_bytes`; `hir_to_lir.cpp` has the
monolithic live implementation used by module lowering. The helper currently
records parity in the returned `StructuredLayoutLookup` object; it does not
emit diagnostics or alter lowering behavior. `LirStructDecl` does not carry
size or alignment, so alignment remains legacy-authoritative in this packet.

## Proof

Delegated proof passed and wrote `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_.*|llvm_gcc_c_torture_src_(align_[123]_c|struct_(cpy|ini|ret)_[0-9]_c|strct_.*_c|stkalign_c))'; } > test_after.log 2>&1`.

Amended proof rerun after touching `hir_to_lir.cpp` to make the exact proof log
show compile evidence. `test_after.log` now begins with rebuilding
`src/codegen/CMakeFiles/c4c_codegen.dir/lir/hir_to_lir/hir_to_lir.cpp.o`,
relinking `c4c_codegen.a`, affected frontend LIR tests, and `c4cll`; then
25/25 selected tests passed.
