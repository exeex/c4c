Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prefer Structured Lookup For Size And Alignment Helpers

# Current Packet

## Just Finished

Completed `plan.md` Step 3 caller-migration packet. Threaded the active
`LirModule*` into `build_fn_signature` so both declaration and definition
byval aggregate alignment text now reaches the shared structured layout helper
through `object_align_bytes(mod, lir_module, ...)`. Also migrated global
variable alignment lowering to the module-aware overload. The legacy
no-module overload remains available and still delegates with a null module,
so existing fallback behavior remains unchanged.

## Suggested Next

Next packet can evaluate whether alloca hoisting should receive a
`LirModule*` through `init_fn_ctx` / `hoist_allocas`, covering modified
parameter slots and local variable stack alignment without changing fallback
semantics.

## Watchouts

The remaining legacy-only live alignment calls in `hir_to_lir.cpp` are inside
alloca hoisting. `build_fn_signature` keeps a default null `LirModule*` in the
header to preserve old call-site behavior for any future or external local
uses. `LirStructDecl` still does not carry size or alignment, so alignment
remains legacy-authoritative while structured lookup coverage is expanded.

## Proof

Delegated proof passed and wrote `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_.*|llvm_gcc_c_torture_src_(align_[123]_c|struct_(cpy|ini|ret)_[0-9]_c|strct_.*_c|stkalign_c))'; } > test_after.log 2>&1`.

The selected subset passed: 25/25 tests.
