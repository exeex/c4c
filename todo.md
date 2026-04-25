Status: Active
Source Idea Path: ideas/open/100_hir_compile_time_template_consteval_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-read lookup paths

# Current Packet

## Just Finished

Step 4 implementation packet completed in `src/frontend/hir/compile_time_engine.hpp`: routed the existing `find_template_struct_specializations(const Node* primary_def)` overload through the structured owner lookup.

The overload still preserves the existing null and `primary_def->name` guard, and now calls `find_template_struct_specializations(primary_def, primary_def->name)` so the structured key is tried first with the rendered primary name as the legacy fallback.

## Suggested Next

Next coherent packet: route one additional declaration-aware lookup path to an existing structured helper where the declaration pointer and rendered fallback are already in hand.

## Watchouts

This packet intentionally changed only the `const Node*` template struct specialization lookup overload. The string-only lookup and registration paths remain available as rendered-name fallbacks, and no emitted names, diagnostics, or tests were changed.

## Proof

Proof succeeded and was written to `test_after.log`.

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_positive_sema_template_constexpr_member_runtime_cpp|cpp_positive_sema_if_constexpr_template_chain_cpp)$') > test_after.log 2>&1
```

Result: build completed and all 3 selected tests passed.
