# `types.cpp` extraction

## Purpose and current responsibility

This file is the front door from raw x86 assembly text into the peephole pipeline's compact per-line metadata. Its job is not optimization itself; it performs cheap string parsing and classification so later passes can reason about barriers, register traffic, `%rbp` stack-slot traffic, and a few instruction families without reparsing whole lines repeatedly.

In practice it owns three things:

- line normalization helpers over `std::string_view`
- register-family and stack-slot recognizers
- `classify_line`, which converts one textual line into `LineInfo`

The file is intentionally shallow and table-free except for a fixed GP-register list. It assumes AT&T-syntax x86 text and encodes just enough structure for downstream peephole passes.

## Important APIs and contract surfaces

The public surface exported through `peephole.hpp` is small and string-oriented:

```cpp
LineInfo classify_line(std::string_view raw);
std::uint16_t scan_register_refs(std::string_view s);
std::uint8_t register_family_fast(std::string_view reg);
std::string_view trim_spaces(std::string_view text);
std::string_view trailing_operand(std::string_view text);
```

`classify_line` is the main contract. It fills a `LineInfo` record that downstream passes treat as cached evidence:

```cpp
struct LineInfo {
  LineKind kind;
  ExtKind ext_kind;
  std::uint16_t trim_start;
  RegId dest_reg;
  bool has_indirect_mem;
  std::int32_t rbp_offset;
  std::uint16_t reg_refs;
};
```

Observed contract expectations:

- `trim_start` preserves the original left trim amount so rewritten lines can keep indentation.
- `kind` is a coarse semantic bucket, not a full instruction decode.
- `dest_reg` is best-effort and often derived from the trailing operand.
- `reg_refs` is a 16-bit mask over GP register families, not exact operand structure.
- `rbp_offset` is meaningful only for recognized `%rbp` frame accesses; otherwise it stays `RBP_OFFSET_NONE`.
- `has_indirect_mem` is a quick hazard bit for non-`%rbp` indirect memory accesses.

Internal helper surfaces that matter structurally:

```cpp
static std::int32_t fast_parse_i32(std::string_view text);
static bool is_conditional_jump(std::string_view s);
std::optional<StoreRbpParts> parse_store_to_rbp_str(std::string_view s);
std::optional<LoadRbpParts> parse_load_from_rbp_str(std::string_view s);
std::optional<std::string_view> parse_setcc(std::string_view s);
```

These helpers define the recognizers that `classify_line` relies on for the higher-value line kinds.

## Dependency direction and hidden inputs

Dependency direction is one-way: this file depends on `peephole.hpp` types and tiny inline string helpers, then produces `LineInfo` for the rest of the peephole subsystem. Downstream passes depend on these summaries being stable and cheap.

Hidden inputs and assumptions:

- assembly is AT&T flavored: registers are `%rax`, `%eax`, `%al`, etc.
- instruction spelling is matched textually (`movq `, `pushq `, `setcc`, `jmp `, `ret`)
- `%rbp` stack accesses are recognized only in narrow textual shapes
- indirect memory is treated specially unless it is `%rbp`-based
- register scanning uses substring search, so fidelity is intentionally approximate
- condition-code jumps are recognized from a hardcoded prefix list rather than an opcode table

This means the file is more of a fast lexical classifier than a parser. Later passes inherit that approximation budget.

## Responsibility buckets

### 1. Cheap text cleanup

- compute left trim
- trim surrounding spaces/tabs
- extract trailing operand after the last comma
- parse signed decimal offsets with a permissive digit loop

### 2. Register-family summarization

- `scan_register_refs` maps any seen GP register spelling to a 16-bit family mask
- `register_family_fast` collapses register aliases (`rax/eax/ax/al/ah`) into a single `RegId`

This deliberately discards width and exact operand role.

### 3. Frame-access recognition

- `parse_store_to_rbp_str` recognizes `mov{q,l,w,b}` into `offset(%rbp)`
- `parse_load_from_rbp_str` recognizes loads from `offset(%rbp)` including `movslq`
- `parse_rbp_offset` extracts the numeric displacement for `%rbp` references

These are the key hooks that let later passes reason about locals and temporaries.

### 4. Line-kind classification

`classify_line` applies ordered tests:

- empty / label / directive
- `%rbp` stores and loads
- self-`movq`
- control-flow barriers (`ret`, `jmp`, conditional jump, `call`)
- compare / push / pop / `setcc`
- fallback `Other`

Order matters because this is a recognizer cascade, not a declarative decode table.

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- `fast_parse_i32` skips full numeric parsing and stops at the first non-digit.
- register scanning uses a fixed 16-register array and substring checks.
- `classify_line` short-circuits on a small set of high-value shapes before falling back to `Other`.

These choices optimize for throughput across many assembly lines, not full correctness under arbitrary syntax.

### Compatibility paths

- `register_family_fast` accepts multiple width aliases for each GP family.
- `%rbp` load/store parsing handles a limited set of move widths.
- `parse_setcc` and conditional-jump detection are string-shape recognizers rather than instruction-model objects.

These are compatibility shims for the code generator's emitted text, not generalized assembly parsing.

### Overfit pressure

This file is vulnerable to gradual accretion of more string-shaped recognizers whenever a downstream pass wants one more fact. Existing signs:

- hardcoded conditional-jump spellings
- special `%rbp` load/store recognizers
- self-move detection only for `movq`
- indirect-memory handling built around substring exclusions such as `%rbp`

That pressure is manageable while the classifier remains narrow, but a rebuild should resist turning this into an ever-growing opcode exception list.

## Rebuild ownership

This file should own:

- cheap lexical classification of one line of generated x86 text
- compact metadata extraction needed broadly across peephole passes
- stable, low-cost helpers for register-family and stack-slot summaries

This file should not own:

- pass-specific optimization policy
- an expanding catalog of one-off opcode special cases
- full instruction parsing or operand modeling
- cross-line reasoning, dataflow, or rewrite decisions
