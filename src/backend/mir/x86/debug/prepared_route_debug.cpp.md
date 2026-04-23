# `prepared_route_debug.cpp` contract

## Legacy Evidence

The extraction below records how the old route-debug implementation evolved
into a lane classifier with significant duplicated backend knowledge.

## Purpose and current responsibility

This file is a diagnostic classifier for the x86 prepared-module handoff. It does not perform normal lowering itself. Instead, it inspects a `PreparedBirModule`, probes the current x86 route families in roughly the same order the backend uses, and emits a human-readable summary or trace that explains:

- which module-level or function-level lane matched
- which lane rejected the shape last
- whether the failure looks like an unsupported prepared shape, a missing prepared contract, or a backend exception
- which downstream codegen surface should be inspected next

The file is effectively an evidence generator for route selection and rejection quality across the prepared x86 backend.

## Primary API surface

The public surface is intentionally small. Everything else is private reporting logic.

```cpp
[[nodiscard]] std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt);

[[nodiscard]] std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function = std::nullopt,
    std::optional<std::string_view> focus_block = std::nullopt);
```

Contract notes:

- Input is a fully prepared module, not raw MIR/BIR.
- Output is plain text, not structured data.
- `summary` compresses to lane result plus final rejection.
- `trace` includes every attempted lane and its detail.
- Focusing is string-based and optional: function name, block label, and an environment-driven value focus.

## Important internal contract surfaces

The core private entry point is one renderer parameterized by verbosity:

```cpp
std::string render_route_report(const c4c::backend::prepare::PreparedBirModule& module,
                                RouteDebugVerbosity verbosity,
                                RouteDebugFocus focus);
```

That renderer depends on three internal report shapes:

```cpp
struct FunctionRouteAttempt { std::string lane_name; bool matched; ... };
struct FunctionRouteReport { std::string function_name; ... std::vector<FunctionRouteAttempt> attempts; };
struct FinalRejectionReport { FinalRejectionKind kind; const FunctionRouteAttempt* attempt; ... };
```

These are not backend IR contracts. They are debug-report assembly state.

## Dependency direction and hidden inputs

The file sits above multiple x86 route helpers and consumes them as black-box probes. It depends on:

- `prepared_route_debug.hpp` for the public surface
- x86 ABI helpers for target triple/profile, register naming, and asm symbol rendering
- prepared-query helpers for resolving prepared names, control-flow records, addressing records, and value-location records
- x86 route emitters such as:
  - trivial defined-function rendering
  - local arithmetic guard renderers
  - countdown-entry renderer
  - local-slot guard-chain renderer
  - single-block return dispatch renderer
  - prepared module multi-defined dispatch builder

Important hidden inputs:

- `C4C_MIR_FOCUS_VALUE` environment variable adds prepared-value filtering without appearing in the public API.
- Rejection classification is string-driven: details thrown by downstream route code are parsed for words like `only supports`, `requires`, `authoritative`, `metadata`, and `handoff`.
- “Next inspect” guidance is hardcoded to source-file paths and lane names, so diagnostics depend on naming stability across the backend.

## Responsibility buckets

### 1. Text formatting and report assembly

- indentation helpers
- singular/plural label generation
- fact joining and rendering
- summary vs trace formatting differences

### 2. Lane-shape fact extraction

The file computes compact facts for several narrowly recognized shapes:

- single-block void call sequences
- bounded same-module variadic helpers
- aggregate forwarding wrappers
- floating aggregate call helpers
- floating aggregate sret copyout helpers
- same-module scalar helper-wrapper families
- single-block i64 immediate-return helpers
- single-block i64 arithmetic-right-shift helpers
- compare-driven entry candidates

These fact builders are mostly pattern recognizers over BIR instructions and function metadata.

### 3. Rejection normalization

- classifies rejection into ordinary miss, unsupported shape, missing contract, or backend exception
- synthesizes one final rejection per function from the last meaningful failing lane
- maps lane names to “next inspect” backend files
- derives module-level rejection text for bounded multi-function dispatch

### 4. Prepared-module lookup glue

- resolves prepared arch and target profile
- locates prepared control-flow, addressing, and value-location functions
- computes focused block/value subsets
- bridges from function names to prepared records

### 5. Lane probing orchestration

`render_route_report` builds a dispatch context, then probes current route families in sequence. It records whether each lane matched, rejected with detail, or was irrelevant.

The main function-level lane order is effectively:

- `trivial-defined-function`
- `local-i32-arithmetic-guard`
- `countdown-entry-routes`
- `local-i16-arithmetic-guard`
- `local-slot-guard-chain`
- `single-block-return-dispatch`
- `compare-driven-entry`
- synthetic rejection-only helper families:
  - bounded same-module variadic helper
  - single-block void call sequence
  - single-block i64 ashr helper
  - single-block i64 immediate helper
  - floating aggregate sret copyout helper
  - floating aggregate call helper
  - aggregate forwarding wrapper
  - same-module scalar call-wrapper family

The module-level bounded multi-function lane is evaluated separately before per-function reporting.

## Fast paths, compatibility paths, and overfit pressure

### Current fast paths

- Early exit for non-x86 targets.
- Tiny public API over one renderer.
- Existing backend renderers are used as match probes by checking whether they return rendered asm.
- Minimal helper lambdas supply just enough ABI/global/string context for those probes.

### Compatibility paths

- The file knows about both `x86_64` and `i686`.
- It supports debug focus by function, block, and prepared value even though those are observational concerns, not codegen concerns.
- Several helper-lane builders exist solely to explain why currently unsupported prepared shapes are close to recognized families.
- Rejection summaries point callers to current implementation files rather than a stable interface layer.

### Overfit pressure

This file has strong shape-enumeration pressure. The warning signs are:

- many narrowly named helper families encoded directly into diagnostics
- string-matched exception text used as semantic classification
- hardcoded routing from lane names to implementation files
- duplicated knowledge of backend support boundaries that likely already exists in emitter code
- synthetic “recognized but unsupported” lanes that are close to testcase-shaped taxonomy

The file is useful as evidence, but it is also a magnet for adding one more pattern detector whenever a new unsupported case appears.

## Design Contract

Current rebuild role:

- provide the stable `summary` and `trace` debug surface for prepared x86 route
  observation
- report ownership-level information such as target, function inventory, and
  high-level lane status
- stay observational rather than becoming a second lowering path

Owned inputs:

- `PreparedBirModule`
- optional function/block focus filters

Owned outputs:

- human-readable summary text
- human-readable trace text

Must not own:

- semantic support decisions duplicated from emitters
- exception-string parsing as the long-term contract
- source-file path routing as primary API

Deferred gaps:

- structured lane diagnostics and fine-grained route rejection reasons are
  postponed to behavior recovery

## Rebuild ownership guidance

### Should own

- the thin public “summarize/trace prepared route decisions” surface
- stable report assembly for module/function/lane outcomes
- translation from backend probe results into user-facing diagnostic text

### Should not own

- backend semantic support decisions duplicated from emitters
- fragile parsing of exception strings as the long-term contract
- ever-growing catalogs of shape-specific unsupported helper families
- source-file path routing as a primary interface
- prepared-module probing logic that would be better exposed as structured backend diagnostics
