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

## Active

### Step 4: Normalize Template Argument Disambiguation Order
- **Done**: Slice 1 — normalized try-order to type-id → expression
- **Remaining**: template-template argument support (low priority for this project's C++ subset)
- **Next slice**: Consider whether `parse_non_type_arg` should use the expression parser instead of raw token capture for complex NTTPs

### Step 5: Add Tentative Parsing Around Potential Template-Id Starts
- Expression primary parsing already uses tentative save/restore (expressions.cpp:1094-1097)
- May need expansion for more ambiguous cases

### Step 6: Recovery/Diagnostics Tracking
- Only after core parsing is stable

## Pre-existing Failures (not blocking)
- `cpp_positive_sema_deferred_consteval_nttp_cpp`
- `cpp_positive_sema_mixed_type_nttp_forwarding_cpp`
- `cpp_positive_sema_operator_compare_basic_cpp`
- `cpp_positive_sema_template_nttp_forwarding_depth_cpp`
