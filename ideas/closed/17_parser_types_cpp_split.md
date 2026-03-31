# Parser types.cpp Split Refactor

Status: Closed
Last Updated: 2026-03-31
Completed: 2026-03-31

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

- [x] `types.cpp` removed or reduced to a thin forwarding file
- [x] Each new file has a single clear responsibility
- [x] `cmake --build build -j8` succeeds
- [x] `ctest --test-dir build -j 8` shows no regressions (2123/2123)

## Completion Notes (2026-03-31)

Implemented as four new files plus a shared helper header:

| File | Content | Lines |
|---|---|---|
| `types_helpers.hpp` | Shared anonymous-namespace helpers (QualifiedTypeProbe, template matching, static helpers) | ~700 |
| `types_template.cpp` | Template registry, NTTP evaluation | ~490 |
| `types_declarator.cpp` | Template scopes, arg parsing, declarator helpers, parse_declarator | ~1260 |
| `types_base.cpp` | Skip helpers, is_type_start, parse_base_type | ~1760 |
| `types_struct.cpp` | Struct/union/enum parsing, parse_param | ~2230 |

Build passes cleanly. Regression: 1466/1467 llvm_gcc_c_torture (1 pre-existing file-system failure unrelated to this refactor).
