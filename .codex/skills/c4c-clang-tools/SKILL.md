---
name: c4c-clang-tools
description: Use when working in this repo on C++ code and you need AST-backed symbol queries instead of reading large files by raw text. Covers function signatures, top-level symbols, definition/declaration lookup, direct callers/callees, and type references through the repo-local `c4c-clang-tool` and `c4c-clang-tool-ccdb` binaries.
---

# C4C Clang Tools

Use this skill when a C++ task would benefit from AST-backed queries, especially
for large files such as `src/backend/lowering/lir_to_bir_module.cpp`.

Prefer these tools before opening large C++ files wholesale.

## Binaries

Prefer installed binaries on `PATH`:

- `c4c-clang-tool`
- `c4c-clang-tool-ccdb`

If either binary is missing from `PATH`, first build and install both tools with:

```bash
scripts/build_install_c4c_clang_tools.sh
```

Do not silently skip this install step just because build-local fallback
binaries already exist. The normal repair path for a missing-`PATH` tool is to
run the install script first, then retry the command from `PATH`.

Build-local fallback paths:

- `build/c4c-clang-tools/c4c-clang-tool`
- `build/c4c-clang-tools/c4c-clang-tool-ccdb`

## Which Entrypoint

- Use `c4c-clang-tool-ccdb` for `.cpp` files that appear directly in
  `build/compile_commands.json`
- Use `c4c-clang-tool` for headers or when you need to pass explicit include
  flags

## Commands

### Function Signatures

Use when planning file splits or extracting candidate declarations.

Compile-db route:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/lowering/lir_to_bir.cpp build/compile_commands.json
```

Flags route:

```bash
c4c-clang-tool function-signatures src/backend/lowering/lir_to_bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser
```

### List Symbols

Use when you need top-level symbol inventory in one file.

```bash
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/lowering/lir_to_bir_module.cpp build/compile_commands.json
```

### Find Definition / Declaration

Use when you know a symbol name and need its site.

```bash
c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/lowering/lir_to_bir.cpp lower_to_bir build/compile_commands.json
```

```bash
c4c-clang-tool-ccdb find-declaration /workspaces/c4c/src/backend/lowering/lir_to_bir.cpp lower_to_bir build/compile_commands.json
```

### Function Callees / Callers

Use when grouping helpers or deciding split boundaries.

```bash
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/lowering/lir_to_bir.cpp lower_to_bir build/compile_commands.json
```

```bash
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/lowering/lir_to_bir.cpp try_lower_to_bir_with_options build/compile_commands.json
```

### Type Refs

Use when deciding whether code belongs in `types`, `globals`, `calls`, `phi`,
or `memory`.

```bash
c4c-clang-tool-ccdb type-refs /workspaces/c4c/src/backend/lowering/lir_to_bir_module.cpp AggregateTypeLayout build/compile_commands.json
```

## Practical Workflow

1. Check `c4c-clang-tool` and `c4c-clang-tool-ccdb` on `PATH`.
2. If either is missing, run `scripts/build_install_c4c_clang_tools.sh`, then
   retry from `PATH`.
3. Start with `function-signatures` or `list-symbols` to get structure.
4. Use `function-callees` and `function-callers` to find dependency clusters.
5. Use `find-definition` / `find-declaration` to confirm move targets.
6. Use `type-refs` before splitting shared structs, aliases, or aggregate
   helpers.
7. Only fall back to reading long files when the AST query result is
   insufficient.

## Current Limits

- `c4c-clang-tool-ccdb` is translation-unit scoped and expects the queried file
  to exist directly in `build/compile_commands.json`
- headers usually still need the flags-driven entrypoint
- callers/callees are direct only
- results are intentionally filtered toward repo-local symbols, not full STL
  call trees
