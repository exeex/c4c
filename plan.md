# c4 / cpp Mode Refactor Implementation Plan

## Goal

Turn the architectural direction in `c4_cpp_mode_plan.md` into a concrete, behavior-preserving implementation sequence.

This plan is for the refactor only:

- no `.cpp` syntax support yet
- no `.c4` syntax support yet
- no parser fork
- no semantic feature expansion yet
- existing `.c` behavior must remain unchanged

The target end state of this plan is:

- frontend mode is explicit at translation-unit entry
- lexer keyword classification is profile-aware
- sema entrypoint is profile-aware and structurally layered
- include handling can carry/infer profile context
- `.h` / `.hpp` policy is representable in code, even if feature use stays minimal

## Ground Rules

- Preserve current C CLI behavior for `.c` inputs.
- Keep lexer, parser, and AST shared.
- Prefer introducing new enums/context objects before moving logic.
- Land this in small, reviewable steps with test coverage after each stage.
- Do not add placeholder syntax parsing for templates or c4-only constructs.
- Do not duplicate C semantic logic in new profile-specific entrypoints.

## Current Code Touch Points

Primary files likely involved:

- `src/frontend/driver.cpp`
- `src/frontend/preprocessor/preprocessor.hpp`
- `src/frontend/preprocessor/preprocessor.cpp`
- `src/frontend/preprocessor/pp_include.cpp`
- `src/frontend/lexer/token.hpp`
- `src/frontend/lexer/token.cpp`
- `src/frontend/lexer/lexer.hpp`
- `src/frontend/lexer/lexer.cpp`
- `src/frontend/sema/sema.hpp`
- `src/frontend/sema/sema.cpp`
- `src/frontend/sema/validate.cpp`
- parser files only if constructor signatures or token plumbing need light adaptation

Suggested new files:

- `src/frontend/source_profile.hpp`
- optionally `src/frontend/source_profile.cpp`

If the codebase prefers fewer new files, the profile enums can live in an existing frontend header, but they should not be buried inside lexer-only or sema-only headers.

## Implementation Phases

### Phase 0: Define Shared Profile Model

Objective:
Introduce the core enums and mapping helpers without changing behavior.

Work:

1. Add explicit profile types in a shared frontend header:
   - `SourceProfile`
   - `LexProfile`
   - `SemaProfile`
2. Add conversion helpers:
   - file extension -> `SourceProfile`
   - `SourceProfile` -> `LexProfile`
   - `SourceProfile` -> `SemaProfile` where valid
3. Add helpers for header classification:
   - `.c` -> C translation unit
   - `.cpp` -> cpp-subset translation unit
   - `.c4` -> c4 translation unit
   - `.h` -> shared C-family header
   - `.hpp` -> shared cpp-family header
4. Keep unknown-extension handling conservative:
   - either preserve current behavior
   - or default only where current driver already assumes C

Deliverable:

- A compile-time profile vocabulary exists and is not yet required by the rest of the pipeline.

Acceptance criteria:

- Build passes.
- No CLI behavior changes.

Notes:

- Keep `SourceProfile` and `SemaProfile` separate even if they map 1:1 for translation units right now.
- This phase should not modify lexer or sema behavior yet.

### Phase 1: Thread Translation-Unit Profile Through Driver

Objective:
Make frontend mode explicit at entry without changing parsing or validation results for `.c`.

Work:

1. Update `src/frontend/driver.cpp` to determine source profile from input path.
2. Introduce a small per-translation-unit context if useful:
   - input path
   - `SourceProfile`
   - `LexProfile`
   - `SemaProfile`
3. Continue using one parser path.
4. Keep `.c` as the only fully exercised path, but make `.cpp` and `.c4` flow through the same pipeline shape.

Deliverable:

- Driver no longer hardcodes an implicit C-only pipeline.

Acceptance criteria:

- `.c` inputs behave exactly as before.
- `--lex-only`, `--parse-only`, and normal codegen still work on `.c`.
- Profile selection is visible in code review and testable.

Suggested tests:

- Add unit tests for extension-to-profile mapping.
- If there is no test harness yet for driver behavior, add a small focused test around mapping helpers instead.

### Phase 2: Refactor Lexer to Use `LexProfile`

Objective:
Make keyword recognition profile-sensitive while keeping one lexer implementation.

Work:

1. Extend `Lexer` constructor to accept `LexProfile`.
   - Keep a compatibility overload temporarily if that reduces churn.
   - Final target should make profile explicit at call sites.
2. Update `keyword_from_string(...)` in `token.hpp` / `token.cpp` to accept `LexProfile`.
3. Split keyword policy into:
   - always-on C keywords
   - cpp-subset-only reserved keywords
   - c4-only reserved keywords
4. For this phase, add only the policy structure and the cpp placeholder keywords named in the design:
   - `template`
   - `constexpr`
   - `consteval`
5. Ensure those tokens are reserved only in:
   - `LexProfile::CppSubset`
   - `LexProfile::C4`
6. Keep c4-only keyword set empty or scaffolded but inactive.
7. Do not change operator lexing:
   - `>>` stays `GreaterGreater`
   - `<<` stays `LessLess`

Deliverable:

- Lexer behavior varies by profile only for keyword classification.

Acceptance criteria:

- Existing C tokenization is unchanged for `.c`.
- In C mode, `template` lexes as `Identifier`.
- In cpp-subset mode, `template` lexes as a keyword token.
- No parser changes are required for `.c` inputs.

Suggested implementation detail:

- If adding new token kinds for future keywords causes parser churn, it is acceptable to add them now but leave parser support absent.
- The parser should only encounter them in `.cpp` or `.c4` paths, which are not expected to succeed yet.

Suggested tests:

- Lexer unit tests for `template`, `constexpr`, `consteval` under:
   - C
   - cpp-subset
   - c4
- Regression test that core C keywords remain keywords in all profiles.

### Phase 3: Make Sema Profile-Explicit Without Forking Logic

Objective:
Replace the implicit single-mode sema entrypoint with a profile-aware one, while keeping existing C validation and HIR lowering as the base layer.

Work:

1. Update `src/frontend/sema/sema.hpp`:
   - add `analyze_program(const Node* root, SemaProfile profile)`
   - optionally keep old overload as a thin wrapper to `SemaProfile::C` during transition
2. Rework `src/frontend/sema/sema.cpp` so that:
   - common C validation always runs first
   - HIR lowering remains shared
   - future extension hooks have a clear insertion point
3. Introduce internal layering helpers, for example:
   - `analyze_program_base_c(...)`
   - `apply_cpp_subset_extensions(...)`
   - `apply_c4_extensions(...)`
4. For now, cpp and c4 extension layers should be no-op or reject-unimplemented in a narrowly scoped way, but they must not duplicate C behavior.
5. Update driver to pass `SemaProfile`.

Deliverable:

- Sema architecture now makes profile explicit and future extension points concrete.

Acceptance criteria:

- `.c` behavior is unchanged.
- C validation and HIR lowering are still the only real semantics executed.
- The code structure makes it obvious that cpp builds on C and c4 builds on cpp.

Suggested tests:

- Existing semantic regression tests still pass in C mode.
- Add focused tests that `analyze_program(..., SemaProfile::C)` matches previous behavior.

### Phase 4: Introduce Include/Profile Context Plumbing

Objective:
Make include processing capable of carrying active source profile, especially for `.h` / `.hpp` policy.

Work:

1. Extend preprocessor include handling to know the active includer profile.
2. Add profile-aware include resolution/classification helpers:
   - included file path -> `SourceProfile`
   - effective profile inherited from includer when file is `.h`
   - `.hpp` inherits includer profile but rejects C mode
3. Decide where the effective profile lives during preprocessing:
   - include stack metadata
   - preprocess context object
   - side-channel per-file profile map
4. Ensure enough information survives to the lex/parse/sema handoff.
5. Emit a clean diagnostic path for:
   - `.hpp` included under C mode

Deliverable:

- The pipeline can represent header mode inheritance correctly.

Acceptance criteria:

- `.h` can be classified as inheriting active mode.
- `.hpp` under C mode produces a deterministic diagnostic.
- Pure `.c` workflows without `.hpp` remain unchanged.

Notes:

- This phase may expose that the current preprocessor returns only flattened text and loses per-file context.
- If so, add the smallest metadata channel that preserves current architecture while enabling profile checks.
- Do not redesign the whole preprocessor in this task.

Suggested tests:

- C TU including `.h` stays accepted.
- C TU including `.hpp` fails with a targeted diagnostic.
- cpp-subset TU including `.hpp` reaches lexer/parser pipeline shape successfully, even if later parsing fails on unimplemented syntax.

### Phase 5: Tighten API Surface and Remove Transitional Shims

Objective:
Finish the refactor cleanly so future feature work starts from explicit mode-aware APIs.

Work:

1. Remove temporary compatibility overloads if added earlier:
   - `Lexer(std::string source)` shim
   - `analyze_program(const Node* root)` shim
2. Ensure all top-level frontend call sites pass explicit profiles.
3. Audit for hidden C assumptions in helper code.
4. Document the final mode flow in a short developer note or code comment near the shared profile definitions.

Deliverable:

- No major frontend stage relies on implicit C mode anymore.

Acceptance criteria:

- Profile is explicit at translation-unit entry, lexing, and sema entry.
- No dead compatibility scaffolding remains unless intentionally retained for API stability.

## Recommended Stage Breakdown for Another Agent

If this needs to be split into reviewable PR-sized chunks, use this order:

1. Shared profile enums and extension mapping helpers.
2. Driver wiring for explicit profile selection.
3. Lexer constructor and keyword table refactor.
4. Lexer tests for profile-sensitive keywords.
5. Sema signature refactor with shared base pipeline.
6. Include-context/profile propagation in preprocessor.
7. `.hpp` rejection and header inheritance tests.
8. Cleanup pass removing temporary shims.

This keeps semantic risk low and makes regressions easier to isolate.

## File-Level Task Checklist

### `src/frontend/driver.cpp`

- Infer source profile from input file extension.
- Pass explicit `LexProfile` into `Lexer`.
- Pass explicit `SemaProfile` into `analyze_program`.
- Keep current CLI semantics unchanged for `.c`.

### `src/frontend/lexer/token.hpp` and `src/frontend/lexer/token.cpp`

- Add profile-aware keyword lookup.
- Add token kinds for future cpp keywords if needed.
- Keep C keyword behavior stable.

### `src/frontend/lexer/lexer.hpp` and `src/frontend/lexer/lexer.cpp`

- Store `LexProfile` in lexer state.
- Route identifier scanning through profile-aware keyword lookup.
- Avoid any parser-style context sensitivity.

### `src/frontend/sema/sema.hpp` and `src/frontend/sema/sema.cpp`

- Add profile-explicit analysis entrypoint.
- Keep C validation/lowering as the base layer.
- Create no-op extension hooks for cpp and c4.

### `src/frontend/preprocessor/preprocessor.hpp`
### `src/frontend/preprocessor/preprocessor.cpp`
### `src/frontend/preprocessor/pp_include.cpp`

- Carry effective source profile through include expansion.
- Support `.h` profile inheritance.
- Reject `.hpp` under C mode with a clear diagnostic.
- Preserve current flattening behavior as much as possible.

## Validation Plan

Run after each phase where practical:

1. Build the project.
2. Run existing frontend tests.
3. Run focused lexer tests for keyword-profile behavior.
4. Run focused include tests for `.h` / `.hpp` policy.
5. For a representative `.c` corpus, compare:
   - lex output
   - parse success/failure
   - semantic diagnostics
   - generated LLVM IR where applicable

Minimum regression bar:

- zero newly failing C tests
- no behavior change for existing `.c` inputs except improved diagnostics where explicitly intended

## Risks and Mitigations

### Risk 1: Parser churn caused by new keyword token kinds

Mitigation:

- Keep parser untouched for C mode.
- Limit new reserved keywords to profiles that are not yet expected to parse successfully.

### Risk 2: Preprocessor currently loses file/profile context

Mitigation:

- Add a narrow metadata channel rather than redesigning preprocessing.
- Keep include-profile checks close to include handling.

### Risk 3: Accidental semantic duplication

Mitigation:

- Implement extension layers as wrappers around the existing C pipeline.
- Make no-op hooks explicit rather than copying validation code.

### Risk 4: Hidden implicit-C assumptions remain after refactor

Mitigation:

- Audit constructor signatures and top-level APIs.
- Remove compatibility shims before declaring the refactor complete.

## Definition of Done

This implementation plan is complete when:

- frontend mode is explicit in driver, lexer, and sema
- keyword recognition is profile-sensitive
- C mode behavior is preserved
- `.h` / `.hpp` mode policy is representable and enforced at include boundaries
- sema is structurally ready for `C -> cpp -> c4` extension layering
- no parser fork or duplicated semantic pipeline has been introduced

## Non-Goals for This Plan

Do not implement in this refactor:

- template parsing
- constexpr semantics
- consteval semantics
- c4 reflection
- c4 module system
- c4-native type system
- headerless module semantics

Those belong to later feature plans after this architectural seam is in place.
