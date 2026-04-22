# `x86_codegen.hpp`

Primary role: mixed shared contract header for canonical x86 lowering, prepared-route dispatch, and helper declarations.

Key surfaces:

```cpp
std::optional<std::string> select_prepared_call_argument_abi_register_if_supported(...);
std::string emit_prepared_module(const c4c::backend::prepare::PreparedBirModule& module);
std::string emit_module(const c4c::backend::bir::Module& module);
```

- Publishes low-level register helpers, prepared ABI selection helpers, dispatch contexts, shared data structures, and top-level entrypoints in one header.
- Acts as the main evidence of responsibility mixing: canonical machine helpers and prepared-route surfaces are not separated.
- Couples most x86 codegen translation units together through broad declarations instead of narrow headers.
- Special-case classification: mixed `core lowering`, `dispatch`, and `legacy compatibility` surfaces in one place; this file is a primary rebuild pressure point.
