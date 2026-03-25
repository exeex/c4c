# Clang-Style Angle Bracket Parse — Execution State

## Completed

### Step 1: Audit Remaining Angle-Bracket Ownership Gaps (2026-03-26)
- Audited all parser files for angle-depth scanning, manual `>>/>=` handling, inline template-arg parsing
- **Close-policy gaps** (skip-balanced-tokens loops using local angle-depth instead of canonical helpers):
  - `types.cpp:3654-3702` — base class scanning (normal path)
  - `types.cpp:3706-3726` — base class scanning (catch fallback)
  - `types.cpp:4799-4821` — field initializer scanning
  - `statements.cpp:75-90` — using alias scanning
  - Note: These are skip-balanced loops, not template arg parsing; token splitting not needed
- **Canonical template-argument parser gaps**:
  - `types.cpp:3596-3630` — struct specialization inline arg parser → **FIXED in Step 3 slice 1**
- **Tentative parsing (probe-ahead, acceptable)**:
  - `types.cpp:3573-3585` — struct specialization probe
  - `types.cpp:3442-3467` — operator qualified name skip
- **Canonical (already correct)**:
  - `parse.cpp:580-628` — canonical close-policy functions
  - `types.cpp:1016-1138` — canonical `parse_template_argument_list()`
  - `types.cpp:923-953` — `find_template_arg_expr_end()`
  - `expressions.cpp:1097` — expression-side template-id (uses canonical path)
  - `declarations.cpp:647-780` — template param defaults (uses canonical close for `>>`)

### Step 3, Slice 1: Replace struct specialization inline arg parser (2026-03-26)
- Replaced inline template-argument mini-parser in `parse_struct_or_union()` with canonical `parse_template_argument_list()`
- Passed `find_template_struct_primary(tag)` as `primary_tpl` to preserve NTTP-vs-type disambiguation
- No regressions: 2123/2127 (same 4 pre-existing failures)

### Step 2: Finish Parser-Owned Template Close Handling (2026-03-26)
- Skip-balanced-tokens loops (base class scanning, field init, using alias) use local angle-depth
- These don't need `parse_greater_than_in_template_list()` (they skip, not consume template args)
- **Decision**: Leave as-is; they are correct skip-balanced loops, not template argument parsing

### Step 3: Build One Canonical Template Argument Parser Path (2026-03-26)
- struct specialization inline parser migrated to canonical path (slice 1)
- No other inline template-argument parsers identified; all callers use canonical path
- **Status**: Complete

### Step 4, Slice 1: Normalize disambiguation order (2026-03-26)
- Changed `parse_template_argument_list()` from conditional try-order (skip type when expect_value) to normalized Clang-style order: always try type-id first, then non-type expression
- `clearly_value` optimization: when `primary_tpl` says NTTP and token is unambiguously a value (IntLit, CharLit, true, false, `-`, Identifier), skip redundant type attempt
- For non-obvious NTTP tokens (parenthesized expressions, sizeof, etc.), type-id is now tried first before non-type — matches Clang `ParseTemplateArgument()` convention
- No regressions: 2123/2127 (same 4 pre-existing failures)

### Step 5, Slice 1: Fix template arg expr scan crossing statement boundaries (2026-03-26)
- **Root cause**: `find_template_arg_expr_end()` did not stop at `;` or unbalanced `}`
- When expression parser tentatively tried `x<...>` template-id for `x < other.x;` inside a method body, the scan crossed `;` and `}` until it found `>` in `operator>` on the next line
- **Symptom**: `operator_compare_basic.cpp` failed — `operator>(Val other)` parsed as function call args after false template match
- **Fix**: Added `Semi` break and unbalanced `RBrace` break to `find_template_arg_expr_end()`
- **Result**: 2124/2127 (+1), `operator_compare_basic` now passes

### Step 4, Slice 2: Reject phantom type-id for bare NTTP identifiers (2026-03-26)
- **Root cause**: `parse_type_arg` inside `parse_template_argument_list()` would succeed for bare identifiers like `N` in `leaf<N>()` — `parse_type_name()` returned default `int` with `N` consumed as declarator name
- **Symptom**: Forwarded NTTP args classified as type args → NTTP bindings empty → deferred template calls unresolved → consteval not evaluated; multi-hop NTTP forwarding produced wrong mangled names
- **Fix**: After `parse_type_name()` in `parse_type_arg`, reject when exactly one Identifier token was consumed and that identifier is not a known type name or template type parameter
- **Result**: 2127/2127 (all passing!) — fixes 3 pre-existing failures:
  - `deferred_consteval_nttp` — consteval in deferred NTTP chains now evaluates
  - `mixed_type_nttp_forwarding` — mixed type+NTTP deferred chains work
  - `template_nttp_forwarding_depth` — multi-hop NTTP forwarding resolves correctly

### Step 5: Add Tentative Parsing Around Potential Template-Id Starts (2026-03-26)
- **Done**: Slice 1 — fixed `find_template_arg_expr_end` statement boundary stops
- Expression primary parsing already uses tentative save/restore (expressions.cpp:1094-1184)
- `is_valid_after_template()` lambda validates follow-tokens to distinguish template-id from comparison
- Focus corpus validation test added: nested `A<B<C>>`, deep `A<B<C<D>>>`, `Trait<T>::value` NTTP, `enable_if_t<(N > 0), int>`, `foo<bar, baz>(x)` — all passing
- **Status**: Complete for current C++ subset

### Step 6: Recovery/Diagnostics Tracking (2026-03-26)
- Core parsing is stable; recovery tracking could be added
- **Decision**: Defer — not needed for current use cases; all 2128 tests pass
- **Status**: Deferred (low priority)

### Validation: Focus corpus test (2026-03-26)
- Added `template_angle_bracket_validation.cpp` covering plan's suggested focus corpus
- All patterns parse and execute correctly

## Plan Completion Summary

All "Done" criteria from plan.md are satisfied:
1. ✅ Parser-owned helper is the only place that splits `>`-prefixed tokens for template closing
2. ✅ Template argument parsing goes through one canonical parser path (`parse_template_argument_list`)
3. ✅ `<` as template opener vs operator decided by tentative save/restore + follow-token validation
4. ✅ Template argument disambiguation follows type-id first → non-type expression (Clang order)
5. ✅ Focused regression cases for nested templates and dependent NTTPs stay green (2128/2128)

## Deferred Items (outside plan scope)
- Template-template argument support (not needed for this project's C++ subset)
- `parse_non_type_arg` expression parser migration (raw token capture works correctly)
- `auto` deduction from template struct return types (codegen bug, not parser)
- `pointer_check<T, is_pointer<T>::value>` static method instantiation (codegen/instantiation bug, not parser)

## Final Test Count
- **2128/2128** all passing
