# `frame_compact.cpp`

## Purpose and current responsibility

This file is a peephole pass hook named `compact_frame`, but the current implementation does not perform general frame-slot compaction. It scans assembly lines for a stack-pointer decrement shaped like `subq $..., %rsp` and only removes the line when the immediate is effectively zero. In practice, this is a trivial cleanup pass wearing the name of a much larger optimization.

Its real responsibility today is:

- recognize one narrow stack-adjustment syntax
- convert the line to `LineKind::Nop` when the decrement is already zero
- report whether any line metadata changed

It does not rewrite frame size, track live stack slots, or reason across prologue/epilogue structure.

## Important APIs and contract surfaces

Primary exported surface from this file:

```cpp
bool compact_frame(const LineStore* store, LineInfo* infos);
```

Observed local helper surfaces:

```cpp
void mark_nop(LineInfo& info);
static std::string trimmed_line(const LineStore* store, const LineInfo& info, std::size_t index);
```

Contract assumptions inherited from `peephole.hpp` rather than declared here:

- `store` and `infos` are parallel arrays indexed identically.
- `LineInfo::trim_start` is already computed by classification.
- `LineInfo::kind` must already distinguish `Other` from structured instruction kinds.
- converting a line to `LineKind::Nop` is enough for later pipeline stages to ignore it; the source text is left unchanged.
- helper predicates `starts_with` and `ends_with` define the textual match policy.

## Dependency direction and hidden inputs

This file depends downward on the peephole shared model in `peephole.hpp`:

- `LineStore` provides raw text access.
- `LineInfo` provides preclassified metadata and trim offsets.
- `LineKind` encodes rewrite state.
- string-shape helpers (`starts_with`, `ends_with`) control recognition.

Hidden inputs that materially affect behavior:

- prior line classification must mark the candidate as `LineKind::Other`; any future classifier change can silently disable this pass
- `trim_start` decides what portion of each line is inspected
- textual formatting must remain compatible with `subq $... , %rsp` style matching; semantic equivalents with different spacing or instruction forms are ignored
- the search for `"$0"` is substring-based, so the fast path is tied to text shape rather than parsed immediates

Dependency direction is one-way: this pass consumes precomputed metadata and mutates only `infos[i].kind`.

## Responsibility buckets

### Core lowering

None. This file does not implement a semantic frame-layout transform.

### Optional fast path

- remove no-op stack-pointer decrements when the line text contains `"$0"` and matches the `subq ... %rsp` shell

### Legacy compatibility

- preserve the name and slot of a richer Rust-origin pass while only mirroring a tiny safe subset
- rely on line-metadata rewrites instead of rewriting assembly text directly

### Overfit pressure

- the pass name invites expansion into ad hoc text-pattern handling for more frame cases
- current zero-detection is substring-based, which is easy to extend incorrectly into testcase-shaped matching instead of building parsed stack-adjust semantics
- dependence on `LineKind::Other` means support can accidentally drift based on classifier quirks rather than a stable frame-compaction contract

## Notable fast paths, compatibility paths, and limits

Fast path:

- if a trimmed line starts with `subq $`, ends with `, %rsp`, and contains `"$0"`, mark it as `Nop`

Compatibility path:

- keep the old pass hook alive in the pass pipeline without claiming to solve general frame compaction

Hard limits:

- no parsing of the immediate value
- no stack-slot liveness tracking
- no inspection of surrounding prologue/epilogue lines
- no handling of equivalent instruction spellings or normalized whitespace variants beyond the exact textual shell

## Rebuild ownership

In a rebuild, this area should own only the semantic detection and elimination of provably redundant frame-adjust operations as part of a clearly defined stack-frame optimization seam.

It should not own broad frame-slot compaction policy, classifier-dependent text hacks, or testcase-shaped string matching that substitutes for parsed stack-adjust analysis.
