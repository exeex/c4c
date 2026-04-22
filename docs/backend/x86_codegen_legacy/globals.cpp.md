# `globals.cpp`

Primary role: canonical global-address and global-load/store lowering, plus minimal scalar/global slice emitters.

Key surfaces:

```cpp
void X86Codegen::emit_global_addr_impl(const Value&, const std::string& name);
void X86Codegen::emit_global_load_rip_rel_impl(const Value&, const std::string& name, std::int64_t byte_offset, IrType);
std::optional<MinimalExternGlobalArrayLoadSlice> parse_minimal_extern_global_array_load_slice(const c4c::backend::bir::Function&);
```

- Handles RIP-relative globals, TLS globals, label addresses, and a family of narrow "minimal slice" helpers used for tiny supported cases.
- Shows both canonical ownership and historical narrow-case growth in one file.
- Shares symbol/prelude helpers with `mod.cpp`.
- Special-case classification: `core lowering` for normal global accesses; `optional fast path` for the minimal slice parsers and emitters.
