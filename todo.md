Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: none
Current Step Title: none

# Current Packet

## Just Finished

Completed `plan.md` Step 3 alloca-hoisting caller-migration packet. Threaded
the active `LirModule*` from function-definition lowering into `init_fn_ctx`
and `hoist_allocas`, so modified-parameter spill slots and non-VLA local
stack allocas now reach the shared structured layout helper through
`object_align_bytes(mod, lir_module, ...)`. The legacy no-module defaults
remain available and still delegate with a null module, so fallback behavior
remains unchanged.

## Suggested Next

Supervisor can review Step 3 completion state and decide whether the remaining
legacy dynamic VLA statement-time alignment query belongs in this plan or a
separate follow-up packet.

## Watchouts

`hoist_allocas` and `init_fn_ctx` keep default null `LirModule*` arguments in
the header to preserve old call-site behavior for any future or external local
uses. `StmtEmitter::emit_local_decl` still uses the legacy no-module overload
for dynamic VLA allocas; that path is not part of the hoisted alloca family.
`LirStructDecl` still does not carry size or alignment, so alignment remains
legacy-authoritative while structured lookup coverage is expanded.

## Proof

Delegated proof passed and wrote `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_call_variadic_aggregate_runtime_c|abi_abi_variadic_.*|llvm_gcc_c_torture_src_(align_[123]_c|struct_(cpy|ini|ret)_[0-9]_c|strct_.*_c|stkalign_c))'; } > test_after.log 2>&1`.

The selected subset passed: 25/25 tests.
