# Parser types.cpp Split Refactor

Status: Open
Last Updated: 2026-03-31

## Goal

Split `src/frontend/parser/types.cpp` (6,700+ lines) into smaller, focused files to reduce cognitive load and make tentative parse paths easier to trace.

## Why this plan

`types.cpp` is the largest file in the parser, mixing unrelated concerns: base type specifiers, declarator parsing, struct/union/enum definitions, template argument lists, attribute handling, and typeof. This makes it hard to reason about individual parse paths, and increases the risk of unintended interactions when modifying one area.

## Proposed Split

| New File | Content | Estimated Lines |
|----------|---------|-----------------|
| `types_base.cpp` | `parse_base_type`, specifier combination, typedef resolution, typeof/typeof_unqual | ~1500 |
| `types_declarator.cpp` | `parse_declarator`, function pointer declarators, array dimensions | ~1500 |
| `types_struct.cpp` | struct/union/enum definition, member parsing, bitfield handling | ~2000 |
| `types_template.cpp` | template argument list, NTTP evaluation, template instantiation helpers | ~1500 |

## Constraints

- Pure structural refactor: no logic changes
- All functions remain methods on `Parser` class (declared in `parser.hpp`)
- Add corresponding entries to `CMakeLists.txt`
- Existing tests must pass unchanged after split

## Acceptance Criteria

- [ ] `types.cpp` removed or reduced to a thin forwarding file
- [ ] Each new file has a single clear responsibility
- [ ] `cmake --build build -j8` succeeds
- [ ] `ctest --test-dir build -j 8` shows no regressions (2123/2123)
