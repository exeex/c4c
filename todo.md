Status: Active
Source Idea Path: ideas/open/100_hir_compile_time_template_consteval_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-read lookup paths

# Current Packet

## Just Finished

Step 4 implementation packet completed in `src/frontend/hir/compile_time_engine.hpp`: added structured-capable `const Node*` lookup/query overloads for template function definitions, primary template struct definitions, template struct specialization owners, and consteval function definitions.

The exact helper surface is `has_template_def(const Node*, const std::string& = {})`, `find_template_def(const Node*, const std::string& = {})`, `has_template_struct_def(const Node*, const std::string& = {})`, `find_template_struct_def(const Node*, const std::string& = {})`, `find_template_struct_specializations(const Node*, const std::string&)`, `has_consteval_def(const Node*, const std::string& = {})`, `find_consteval_def(const Node*, const std::string& = {})`, and `is_consteval_template(const Node*, const std::string& = {})`.

The fallback policy is structured-first when `make_compile_time_registry_key(...)` can construct a key from declaration identity, then rendered-name lookup only when the caller supplies a non-empty rendered name. Existing string-only APIs and the existing `find_template_struct_specializations(const Node*)` behavior remain unchanged.

## Suggested Next

Next coherent packet: route a narrow call-site or metadata path to these opt-in declaration-aware helpers where a recovered declaration pointer and rendered fallback are already available.

## Watchouts

The structured mirrors are still best-effort mirrors. New call-site routing should pass a rendered fallback when available and should not remove or weaken the string registry paths. The string-only `register_template_struct_specialization(primary_name, node)` path still updates only the rendered-name map because it does not receive the primary owner declaration needed for an owner structured key.

## Proof

Proof succeeded and was written to `test_after.log`.

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_positive_sema_template_constexpr_member_runtime_cpp|cpp_positive_sema_if_constexpr_template_chain_cpp)$') > test_after.log 2>&1
```

Result: build completed and all 3 selected tests passed.
