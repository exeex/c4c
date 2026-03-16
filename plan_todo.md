# Plan Execution State

## Baseline
- Date: 2026-03-16
- Tests before: 1784/1784 passed (100%)
- Tests after:  1784/1784 passed (100%)

## Active Item
All phases complete.

## Completed
- [x] Phase 0: Define Shared Profile Model
- [x] Phase 1: Thread Translation-Unit Profile Through Driver
- [x] Phase 2: Refactor Lexer to Use LexProfile
- [x] Phase 3: Make Sema Profile-Explicit
- [x] Phase 4: Include/Profile Context Plumbing
- [x] Phase 5: Tighten API Surface

## Phase 0: Define Shared Profile Model
- [x] Create `src/frontend/source_profile.hpp` with SourceProfile, LexProfile, SemaProfile, HeaderKind enums
- [x] Add conversion helpers: source_profile_from_extension, lex_profile_from, sema_profile_from, header_kind_from_extension
- [x] Build passes, no CLI behavior changes

## Phase 1: Thread Translation-Unit Profile Through Driver
- [x] Update c4cll.cpp to infer profile from input extension
- [x] Compute lex_profile and sema_profile from source_profile

## Phase 2: Refactor Lexer to Use LexProfile
- [x] Extend Lexer constructor to accept LexProfile (default: C)
- [x] Update keyword_from_string to accept LexProfile
- [x] Add KwTemplate, KwConstexpr, KwConsteval token kinds (CppSubset/C4 only)
- [x] Driver passes lex_profile to Lexer
- [x] Verified: `template` is IDENT in C mode

## Phase 3: Make Sema Profile-Explicit
- [x] Add analyze_program(root, SemaProfile) signature
- [x] Refactored into analyze_program_base_c + apply_cpp_subset_extensions + apply_c4_extensions
- [x] Extension hooks are no-op for now
- [x] Driver passes sema_profile to analyze_program

## Phase 4: Include/Profile Context Plumbing
- [x] Added set_source_profile/source_profile to Preprocessor
- [x] .hpp rejection under C mode in handle_include
- [x] Driver sets source_profile on preprocessor
- [x] Fixed CMakeLists.txt to add src/frontend to preprocessor test include dirs

## Phase 5: Tighten API Surface
- [x] All pipeline stages receive explicit profiles from driver
- [x] Default parameters retained for backward compatibility (unused driver.cpp)
- [x] No hidden C assumptions in main call sites

## Blockers
(none)

## Notes
- All profiles default to C, preserving existing behavior
- Default parameter values kept on Lexer, keyword_from_string, analyze_program for API stability
- The old driver.cpp is not compiled (not in CMakeLists.txt) — only c4cll.cpp matters
