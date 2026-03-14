# c4 / cpp Mode Refactor Plan

## Goal

Prepare the frontend architecture for future `.cpp` and `.c4` support without implementing those languages yet.

The immediate objective is to separate:

- keyword classification policy
- semantic analysis policy

while keeping:

- lexer shared
- parser shared
- AST shared
- existing C behavior fully preserved

This plan assumes the language relationship is strictly monotonic:

`C -> cpp subset -> c4`

That means:

- `.cpp` is a strict superset of `.c`
- `.c4` is a strict superset of `.cpp`
- later modes must reuse earlier semantic machinery instead of replacing it

This reuse is not optional. It is the mechanism that preserves the superset guarantee.

## Constraints

- No `c4` syntax is implemented yet
- No `cpp` syntax is implemented yet
- The refactor should only introduce the architectural seams needed for future work
- File extension is the source of mode selection for translation units
- `.h` is shared across `.c`, `.cpp`, `.c4`
- `.hpp` is shared across `.cpp`, `.c4`
- `.h` should be interpreted under the includer's active mode
- `.hpp` should reject C mode
- Compatibility with existing C code is the top priority

## High-Level Direction

The frontend should be split conceptually into four layers:

1. Shared text/token/AST pipeline
2. Extension-aware keyword policy
3. Mode-specific semantic policy
4. Reused lowering/codegen beneath sema

Desired flow:

`source -> preprocessor -> lexer(profile-aware keywords) -> parser(shared grammar) -> AST -> sema profile chain -> HIR/codegen`

The main design rule is:

- syntax infrastructure is shared
- semantic capability grows by extension
- newer language modes build on older ones instead of forking

## Core Refactor Principles

### 1. Keyword decisions happen in the lexer, not in the parser

Keyword handling should vary by active source profile.

Examples:

- in `.c`, `template` is an identifier
- in `.cpp`, `template` is a keyword
- in `.c4`, `template` is also a keyword

This keeps parser structure clean and avoids contextual-keyword sprawl.

### 2. Parser remains shared

There should not be separate C / cpp / c4 parsers.

The parser may later gain extension entry points, but it should still operate as one shared parser over one shared token model.

### 3. Sema must be layered, not duplicated

Because the language relationship is strict superset inheritance:

- cpp sema must reuse c sema logic
- c4 sema must reuse cpp sema logic

The architecture should force this reuse structurally.

Bad direction:

- `analyze_c()`
- `analyze_cpp()` reimplements C rules
- `analyze_c4()` reimplements C and cpp rules again

Good direction:

- C sema provides the base rule set
- cpp sema extends or overrides only where needed
- c4 sema extends cpp sema only where needed

### 4. Headers are not their own language

`.h` and `.hpp` are include surfaces, not standalone semantic modes.

Rules:

- `.h` inherits includer mode
- `.hpp` inherits includer mode, but is invalid under C mode

## Proposed Mode Model

Use two distinct concepts:

### Source profile

Determined from file extension and include context.

Possible model:

```cpp
enum class SourceProfile {
  C,
  Cpp,
  C4,
  HeaderCFamily,
  HeaderCppFamily,
};
```

This expresses how a file entered the pipeline.

### Semantic profile

Determines active language rules.

Possible model:

```cpp
enum class SemaProfile {
  C,
  CppSubset,
  C4,
};
```

In practice:

- `.c` -> `SemaProfile::C`
- `.cpp` -> `SemaProfile::CppSubset`
- `.c4` -> `SemaProfile::C4`
- `.h` -> inherit includer profile
- `.hpp` -> inherit includer profile, reject `C`

The exact enum names can vary, but the separation should remain.

## Keyword Table Refactor

## Objective

Make keyword recognition profile-sensitive without splitting the lexer implementation.

## Target shape

Introduce a lexer profile:

```cpp
enum class LexProfile {
  C,
  CppSubset,
  C4,
};
```

Then thread it through lexer construction:

```cpp
class Lexer {
 public:
  Lexer(std::string source, LexProfile profile);
};
```

And make keyword lookup profile-aware:

```cpp
TokenKind keyword_from_string(std::string_view s, LexProfile profile);
```

## Keyword policy rules

### C keywords

Always reserved in all profiles.

### cpp-subset keywords

Reserved in:

- `CppSubset`
- `C4`

Not reserved in:

- `C`

Initial expected examples:

- `template`
- `constexpr`
- `consteval`

### c4 keywords

Reserved only in:

- `C4`

This set should remain empty for now until `.c4` syntax is introduced.

## Implementation note

The lexer should still tokenize operators using normal longest-match rules.
It should not try to understand template grammar or semantic meaning.

For example:

- `>>` remains `GreaterGreater`
- `<<` remains `LessLess`

Template-angle reinterpretation belongs in the parser later.

## Sema Refactor

## Objective

Make sema extensible as a strict inheritance chain:

`C base -> cpp extension -> c4 extension`

## Required property

If C sema has already solved a problem, cpp sema must reuse that solution.
If cpp sema has already solved a problem, c4 sema must reuse that solution.

This is how the implementation stays aligned with the language superset claim.

## Recommended structure

Introduce a shared semantic engine with extension hooks instead of three unrelated entry points.

Possible directions:

### Option A: layered analyzers

```cpp
AnalyzeResult analyze_c(const Node* root);
AnalyzeResult analyze_cpp_subset(const Node* root);
AnalyzeResult analyze_c4(const Node* root);
```

But internally:

- cpp analyzer calls into C analyzer building blocks
- c4 analyzer calls into cpp analyzer building blocks

This is acceptable only if the internal structure guarantees reuse.

### Option B: one analyzer with pluggable rule profile

```cpp
AnalyzeResult analyze_program(const Node* root, SemaProfile profile);
```

And inside:

- common C validation is always run first
- cpp-specific checks/extensions are applied only for `CppSubset` and above
- c4-specific checks/extensions are applied only for `C4`

This is likely the cleaner long-term direction for this codebase.

## Recommended bias

Prefer one shared sema pipeline with profile-specific feature/rule layers over three separate top-level analyzers.

That gives:

- clearer reuse
- fewer duplicated traversals
- easier guarantee that newer modes are supersets

## Suggested internal layering

### Layer 1: common C semantic core

Owns:

- declarations
- type checking
- conversions
- lvalue/rvalue rules
- storage/linkage basics
- constant expression rules already valid in C mode
- existing validation/HIR lowering assumptions

### Layer 2: cpp-subset extension layer

Adds:

- template-related semantic structures
- constexpr / consteval support
- any limited compile-time evaluation extensions

Must reuse Layer 1 for all ordinary C constructs.

### Layer 3: c4 extension layer

Adds:

- reflection
- native type system features
- syntax sugar lowering
- module-oriented semantics

Must reuse Layer 2 and therefore transitively Layer 1.

## What must not happen

- no duplicated C validation logic inside cpp mode
- no duplicated C or cpp validation logic inside c4 mode
- no semantic divergence where a valid C construct stops working under cpp mode
- no semantic divergence where a valid cpp-subset construct stops working under c4 mode unless explicitly deprecated by design

## Parser and Template Preparation

No template feature is being implemented now, but the plan should reserve the correct responsibility split.

### Lexer responsibility

- classify keywords by active lexical profile
- tokenize operators with normal longest-match behavior

### Parser responsibility

- remain shared
- later add template grammar handling
- later reinterpret `>>` as `>` `>` in template-argument context when needed

This means template support does not require lexer forks.

## Include / Header Policy

### `.c`

- translation unit semantic profile is `C`

### `.cpp`

- translation unit semantic profile is `CppSubset`

### `.c4`

- translation unit semantic profile is `C4`

### `.h`

- not a standalone language mode
- inherits active includer profile
- intended as the shared API bridge across C / cpp / c4

### `.hpp`

- not valid under C profile
- valid under `CppSubset` and `C4`
- inherits includer profile

## Module Direction for `.c4`

The long-term direction for `.c4` is module-first and headerless, similar in spirit to Rust.

That implies:

- `.c4` should not be designed around header-driven API exposure
- `.h` remains primarily an interop bridge artifact
- `.c4` module/export semantics should eventually be designed independently from C-family include mechanics

This does not need to be implemented now, but the refactor should avoid assumptions that all frontends are header-centric.

## Execution Plan

### Phase 1: introduce profiles without behavior change

- add `LexProfile`
- add `SemaProfile`
- thread profile selection from file extension at translation-unit entry
- keep `.c` behavior identical to today

Deliverable:

- architecture seam exists
- all existing tests still run in C mode only

### Phase 2: refactor keyword lookup

- make `keyword_from_string()` profile-aware
- keep current C keyword set unchanged
- add empty placeholders or structure for future cpp/c4 keyword sets
- ensure `.c` lexing output remains unchanged

Deliverable:

- lexer can vary keyword tables by profile even before new syntax exists

### Phase 3: refactor sema entrypoint

- change sema entry so profile is explicit
- isolate existing C sema as the guaranteed base layer
- structure code so future cpp and c4 rules can be appended instead of copied

Deliverable:

- sema architecture becomes extension-friendly
- no cpp/c4 rules required yet

### Phase 4: include-context propagation

- ensure included `.h` inherits active profile
- define `.hpp` rejection path for C mode
- ensure preprocessor/frontend handoff preserves enough file/profile context

Deliverable:

- header policy is enforceable later

### Phase 5: future cpp-subset enablement

- add cpp-subset keywords
- add parser support for limited template/constexpr/consteval syntax
- add cpp extension semantic layer reusing C base

### Phase 6: future c4 enablement

- add c4 keywords
- add module/reflection/native type features
- implement c4 semantic layer on top of cpp subset

## Acceptance Criteria For The Refactor

The refactor is successful when:

- lexer remains one implementation
- parser remains one implementation
- keyword recognition is profile-sensitive
- sema profile is explicit
- C mode behavior is unchanged
- architecture makes `C -> cpp -> c4` reuse mandatory rather than optional
- future cpp work can be added by extending C sema
- future c4 work can be added by extending cpp sema

## Non-Goals

This refactor plan does not yet implement:

- cpp templates
- constexpr
- consteval
- c4 reflection
- c4 module system
- c4 native type system

It only creates the architectural shape needed to add those features without breaking the superset model.

## Final Design Rule

The most important rule is:

- newer modes extend older modes
- they do not replace them

If implementation structure ever makes it easy to duplicate C logic in cpp mode, or duplicate cpp logic in c4 mode, the structure is wrong and should be corrected before feature work continues.
