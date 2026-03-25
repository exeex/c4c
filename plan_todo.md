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

## Active

### Step 2: Finish Parser-Owned Template Close Handling
- Skip-balanced-tokens loops (base class scanning, field init, using alias) use local angle-depth
- These don't need `parse_greater_than_in_template_list()` (they skip, not consume template args)
- **Decision**: Extract shared `skip_balanced_until()` helper to deduplicate, or leave as-is since they're correct
- **Next slice**: Consider extracting shared skip-balanced helper if there's a concrete correctness benefit

### Step 3: Build One Canonical Template Argument Parser Path
- **Done**: struct specialization inline parser migrated to canonical path
- **Remaining**: No other inline template-argument parsers identified; all callers use canonical path
- **Status**: Likely complete — verify no other inline parsers exist

## Not Started

### Step 4: Normalize Template Argument Disambiguation Order
- Make canonical `parse_template_argument_list` follow type-id → template-template → expression order
- Current order: type-first (unless primary_tpl says NTTP), then non-type expression
- Missing: template-template argument support

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
