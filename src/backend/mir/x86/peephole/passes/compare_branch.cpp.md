# `compare_branch.cpp`

## Purpose And Current Responsibility

This file implements one local peephole pass that recognizes a boolean-materialization sequence after `cmp`, then rewrites the later `je`/`jne` into a direct conditional branch on the original compare flags. Its practical goal is to delete the temporary `setcc` / zero-extend / stack spill-reload / `test` chain when that chain is only feeding a branch.

The pass is narrow but not trivial: it is responsible for proving that the boolean path has not been reused in a way that would make removing the intermediate operations unsafe.

## Important API Surface

```cpp
bool fuse_compare_and_branch(LineStore* store, LineInfo* infos);
```

Inputs are the mutable line buffer and the already-classified per-line metadata. The pass mutates both:

- rewrites one jump line in `LineStore`
- marks eliminated intermediates as `LineKind::Nop` in `infos`
- relies on `trim_start`, `kind`, and `rbp_offset` already being valid

Local helper surfaces define the file's real working contract:

```cpp
void mark_nop(LineInfo& info);
void replace_line(LineStore* store, LineInfo& info, std::size_t index,
                  const std::string& text);
std::optional<std::string> parse_setcc_text(std::string_view line);
std::string invert_cc(std::string_view cc);
```

And the safety check that prevents deleting stack traffic unless every tracked store is later reloaded inside the same candidate slice:

```cpp
bool has_unmatched_tracked_store(const LineStore* store, const LineInfo* infos,
                                 std::size_t range_start, std::size_t range_end,
                                 const std::int32_t* store_offsets,
                                 std::size_t store_count);
```

## Dependency Direction And Hidden Inputs

Direct dependency direction is downward into the shared peephole layer:

- `LineStore` owns the mutable assembly text
- `LineInfo` carries prior classification results
- `classify_line()` is re-used to recover the original kind of lines already turned into `Nop`
- `starts_with()` and string trimming helpers define the text-matching vocabulary

Hidden inputs:

- The pass assumes an earlier phase filled `infos` consistently with the textual assembly.
- It depends on x86 flag semantics: `cmp` flags must still be the relevant producer for the branch.
- It depends on `%al` / `%eax` / `%rax` conventions in the emitted boolean sequence.
- It depends on bounded local search windows instead of CFG structure.

## Responsibility Buckets

### 1. Candidate discovery

Starts only at lines classified as `Cmp`. It then gathers the next non-`Nop` instructions inside a fixed lookahead window and requires at least:

- `cmp`
- `setcc`
- some optional bridge instructions
- `test %rax/%eax, %rax/%eax`
- `je` or `jne`

### 2. Boolean-sequence normalization

The pass tolerates a specific bridge shape between `setcc` and `test`:

- `movzbq %al, ...` or `movzbl %al, ...`
- stack stores to `rbp` offsets
- reloads from the same `rbp` offsets
- `cltq` or `movslq`

Everything else ends the candidate.

### 3. Safety gating

If stack stores are seen, the pass tracks up to four `rbp` offsets and rejects the fusion when any tracked store lacks a matching reload before the `test`. This is the file's main protection against deleting a materialized boolean that escapes the immediate path.

### 4. Branch rewrite

For `jne target`, it emits `j<setcc-cond> target`.

For `je target`, it emits the inverted condition:

```cpp
e <-> ne
l <-> ge
g <-> le
b <-> ae
```

Then it marks the intermediate sequence as `Nop` and replaces only the branch line text.

## Fast Paths, Compatibility Paths, And Overfit Pressure

### Fast path

The happy path is a compact local sequence with no stack traffic or only matched spill/reload pairs. The fixed non-`Nop` lookahead keeps the pass cheap and purely local.

### Compatibility path

The file carries compatibility for codegen shapes that materialize compare results through `%al`, extend them, optionally spill to frame slots, then re-test them. Supporting both `testq %rax, %rax` and `testl %eax, %eax` is part of that compatibility envelope.

### Overfit pressure

The implementation is heavily shape-driven:

- exact textual matches for `movzbq`, `movzbl`, `cltq`, `movslq`, `je`, `jne`
- hardcoded `%al`/`%eax`/`%rax`
- only four tracked frame slots
- fixed lookahead depth of eight non-`Nop` instructions

That means the pass is vulnerable to becoming a collector for emitter-specific sequence variants rather than a principled "compare flags still dominate branch" rule. In a rebuild, this is the main pressure point to resist.

## Rebuild Ownership Boundary

This logic should own: recognition and safe elimination of redundant compare-to-boolean-to-branch materialization, expressed as a local branch-fusion rule.

This logic should not own: generalized stack dataflow, broad CFG reasoning, arbitrary text-pattern expansion for every new emitter variant, or the directory's main indexing/interface role.
