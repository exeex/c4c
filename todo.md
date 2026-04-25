Status: Active
Source Idea Path: ideas/open/100_hir_compile_time_template_consteval_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-read lookup paths

# Current Packet

## Just Finished

Step 2 first implementation packet completed in `src/frontend/hir/compile_time_engine.hpp`: added HIR-owned `CompileTimeRegistryKeyKind`, `CompileTimeRegistryKey`, and `CompileTimeRegistryKeyHash`, plus `make_compile_time_registry_key` construction from AST declarations.

The structured key records registry kind, AST declaration kind, namespace context id, global qualification, qualifier segment `TextId`s, and unqualified declaration `TextId`. The declaration pointer is included only as a best-effort fallback when text identity is incomplete, currently when the unqualified `TextId` is invalid or any present qualifier segment lacks a valid `TextId`.

`CompileTimeState` now keeps structured mirror maps beside the existing rendered-name maps for template function definitions, primary template struct definitions, template struct specialization owners, and consteval function definitions. Existing string registrations, lookups, iteration, metadata materialization, and diagnostics remain unchanged; the structured maps are dual-written only from registration paths with declaration-node identity, and the string maps remain authoritative compatibility storage.

## Suggested Next

Next coherent packet: add structured lookup/query helpers that can be used after a legacy rendered-name lookup has recovered a declaration pointer, keeping rendered-name lookup as fallback until call metadata carries complete structured identity.

## Watchouts

The structured mirrors are not authoritative yet. Do not route semantic lookup to them until later packets provide complete lookup metadata and a rendered fallback. The string-only `register_template_struct_specialization(primary_name, node)` path intentionally updates only the rendered-name map because it does not receive the primary owner declaration needed for an owner structured key.

## Proof

Proof succeeded and was written to `test_after.log`.

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_positive_sema_template_constexpr_member_runtime_cpp|cpp_positive_sema_if_constexpr_template_chain_cpp)$') > test_after.log 2>&1
```

Result: build completed and all 3 selected tests passed.
