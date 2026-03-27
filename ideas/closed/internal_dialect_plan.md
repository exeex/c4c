# Internal Dialect Plan

## Goal

Make `c4cll` robust enough to parse GNU / libstdc++ / libc++ style system headers
without treating every `__xxx` spelling as a special keyword.

The target is not "recognize more double-underscore names".

The target is:

- recognize the right subset of GNU and implementation syntax,
- keep `__xxx` identifiers classified by role,
- avoid parser state corruption when entering system-header-only constructs,
- unblock standard library headers in a principled way.


## Why This Plan Exists

Recent failures around `std::vector` showed that several independent problems were
being conflated into one bucket:

- `decltype` really is a language token / type-specifier issue,
- `__attribute__` and similar forms are GNU syntax issues,
- `inline namespace` and linkage blocks are parser-structure issues,
- `__true_type` / `__false_type` are not keywords at all, but implementation
  identifiers that must remain ordinary type names once declared,
- builtin-style forms such as `__has_builtin(...)` belong to the preprocessor,
  not the parser.

If we flatten all of these into "support `__xxx` keywords", we will keep
introducing the wrong fix in the wrong layer.


## Clang Reference Model

Clang does **not** solve this in `lib/Lex` with one giant hard-coded keyword list.

Useful reference points:

- `ref/llvm-project/clang/lib/Lex/Preprocessor.cpp`
  - `Initialize()` populates keyword knowledge through the identifier table
  - `LookUpIdentifierInfo()` maps raw identifiers to `IdentifierInfo`
  - `HandleIdentifier()` decides keyword / macro / extension-token behavior
- `ref/llvm-project/clang/lib/Lex/PPMacroExpansion.cpp`
  - handles builtin-like preprocessor entities such as `__has_builtin`
- `ref/llvm-project/clang/lib/Lex/PPDirectives.cpp`
  - handles reserved macro names, keyword-shadowing, and attribute-name rules

The main design lesson for `c4cll`:

- keep lexer classification small and role-driven,
- keep builtin preprocessor forms in the preprocessor,
- keep implementation identifiers as ordinary identifiers unless the language
  grammar says otherwise.


## Non-Goals

This plan does **not** try to:

- emulate Clang's full identifier table architecture,
- support every GNU / Clang / MSVC extension,
- add every `__builtin_*` immediately,
- make every implementation-reserved identifier special in the lexer,
- guarantee full standard library parsing in one step.


## Classification Model

We should classify `__xxx` spellings into four buckets.

### 1. Real Language Tokens

These should become dedicated token kinds because the parser grammar needs them.

Examples:

- `__typeof__`
- `typeof`
- `__attribute__`
- `__alignof__`
- `decltype`
- `asm`

Rule:

- only spellings that alter grammar should become token kinds.


### 2. Builtin Preprocessor Forms

These should stay in the preprocessor layer.

Examples:

- `__has_builtin(...)`
- `__has_feature(...)`
- `__has_extension(...)`
- `__has_attribute(...)`
- `__is_identifier(...)`

Rule:

- if the form is function-like during preprocessing, do not route it through the
  parser as a normal call expression first.


### 3. Builtin Types / Builtin Functions

These are compiler-known names, but not necessarily keywords.

Examples:

- `__builtin_va_list`
- `__builtin_va_arg`
- `__builtin_expect`
- `__builtin_types_compatible_p`

Rule:

- support these through explicit builtin tables and semantic hooks,
- only create token kinds when grammar truly requires it.


### 4. Implementation Identifiers

These are ordinary names used by the standard library implementation.

Examples:

- `__true_type`
- `__false_type`
- `__type`
- `__value`
- `__cxx11`
- `__8`

Rule:

- never classify these as keywords just because they start with `__`,
- they must succeed through normal namespace / type / template lookup.


## Current Gaps

The current frontend appears to be missing or partially missing support in these areas:

- GNU type spellings and specifiers
  - `decltype` was missing as a type-start / type parser path
- namespace syntax used by libstdc++
  - `namespace std __attribute__((...)) { ... }`
  - `inline namespace __cxx11 { ... }`
- linkage specification blocks
  - `extern "C++" { ... }`
- parser resilience when deep system-header constructs are nested
- preprocessor-side builtin feature forms
- consistent distinction between "keyword-like" and "ordinary implementation identifier"


## Phases

## Phase 1: Establish The Classification Boundary

### Objective

Stop treating the problem as "support more `__xxx` keywords".

### Tasks

1. write down the authoritative buckets in frontend comments / docs

- real grammar tokens
- preprocessor builtin forms
- builtin entities
- implementation identifiers

2. audit current `keyword_from_string(...)`

- list every existing `__xxx` spelling mapped to a token kind
- confirm each one is grammar-driven, not convenience-driven

3. create a small denylist of names that must **not** become keywords

- examples: `__true_type`, `__false_type`, `__type`

### Exit Criteria

- we have a stable policy for where new `__xxx` support belongs
- future fixes can be evaluated against that policy


## Phase 2: Complete High-Value GNU Grammar Tokens

### Objective

Support the GNU / C++ token spellings that actually change parse structure.

### Tasks

1. audit and complete tokenization / type-start handling for

- `__typeof__`
- `typeof`
- `__typeof_unqual__`
- `__attribute__`
- `__alignof__`
- `decltype`
- `asm`

2. verify that each token is accepted in all relevant parser entry points

- type parsing
- declaration parsing
- expression parsing
- namespace / declaration attributes where applicable

3. add focused regression tests

- tiny standalone files, not only full `<vector>` cases

### Exit Criteria

- grammar-driving GNU spellings are consistently recognized
- failures move deeper into real semantic gaps instead of stopping at tokenization


## Phase 3: Harden System-Header Structural Parsing

### Objective

Correctly parse the structural wrappers that standard library headers rely on.

### Tasks

1. support namespace attributes reliably

- `namespace std __attribute__((visibility("default"))) { ... }`

2. support inline namespaces reliably

- `inline namespace __cxx11 { ... }`
- version namespaces such as `__8`

3. support linkage-specification blocks robustly

- `extern "C" { ... }`
- `extern "C++" { ... }`

4. add nested-structure stress tests

- linkage block + namespace attribute + inline namespace + template declarations

### Exit Criteria

- entering a system-header wrapper does not corrupt parser state
- declarations inside those wrappers register in the correct scope


## Phase 4: Separate Preprocessor Builtin Forms

### Objective

Move feature-like builtin handling into the preprocessor instead of letting the
parser trip over them later.

### Tasks

1. inventory preprocessor builtin forms used by libstdc++ / libc++

- `__has_builtin`
- `__has_feature`
- `__has_extension`
- `__has_attribute`
- `__is_identifier`

2. implement minimal evaluation support in the preprocessor

- enough to produce stable integer results in `#if`
- prefer conservative false over malformed token streams

3. add directive-expression tests

- direct `#if __has_builtin(...)`
- nested macro wrappers

### Exit Criteria

- feature-like builtin pp forms no longer leak into parser failures
- `#if` paths in system headers become more stable


## Phase 5: Make Implementation Identifiers Work Through Normal Lookup

### Objective

Ensure names such as `__true_type` succeed as ordinary declarations, not as magic tokens.

### Tasks

1. audit type lookup in these contexts

- typedef base-type lookup
- namespace-qualified lookup
- inline-namespace visibility
- template-body local lookup

2. fix scope-registration bugs instead of seeding many library-specific names

- when a struct / class is declared in a namespace, ensure subsequent type lookup
  can find it through normal rules

3. add regression tests using libstdc++-style internal names

- `struct __true_type {}; typedef __true_type __type;`
- nested inside namespace / inline namespace / template

### Exit Criteria

- implementation identifiers work because the symbol table is correct
- we do not need a growing allowlist of library-private names


## Phase 6: Add A Small Compatibility Escape Hatch

### Objective

Provide a narrowly scoped fallback for progress while root-cause fixes are in flight.

### Tasks

1. define strict rules for fallback seeding

- only for extremely common implementation names
- only when they are known to behave as ordinary types
- must be documented as temporary

2. isolate compatibility shims from the main lexer keyword path

- no blanket "all `__xxx` are keywords"
- no broad special-casing in unrelated parser code

3. attach removal criteria to each shim

- remove once ordinary scope / type lookup works

### Exit Criteria

- temporary unblocks remain visible and easy to delete
- we avoid baking implementation accidents into the core grammar


## Suggested Implementation Order

1. finish the classification audit
2. complete grammar-driving GNU token support
3. stabilize namespace attributes / inline namespace / linkage blocks
4. implement preprocessor builtin forms used by standard library headers
5. repair normal lookup for implementation identifiers
6. keep compatibility shims minimal and temporary


## Immediate Next Slice

The next high-value slice for `std::vector` enablement should be:

1. document current `__xxx` token mappings and mark which ones are grammar-driven
2. add focused tests for
   - `namespace std __attribute__((...)) { ... }`
   - `inline namespace __cxx11 { ... }`
   - `extern "C++" { ... }`
   - `typedef __true_type __type;`
3. implement or repair any remaining parser state bugs found by those tests
4. only then revisit library-private fallback seeding if still necessary


## Definition Of Done

This plan is complete when:

- `c4cll` no longer relies on treating arbitrary `__xxx` spellings as keywords,
- GNU grammar spellings are handled in the right layer,
- preprocessor builtin forms are handled in the preprocessor,
- implementation identifiers like `__true_type` work through ordinary lookup,
- standard library headers advance because parsing is structurally correct, not
  because of a growing list of hard-coded private names.
