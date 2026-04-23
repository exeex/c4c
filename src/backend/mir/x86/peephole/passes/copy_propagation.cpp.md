# `copy_propagation.cpp` extraction

## Purpose and current responsibility

This file implements the x86 peephole pass `propagate_register_copies`. Its job is narrow but stateful: walk the classified assembly line stream, remember active register-to-register `movq` aliases, rewrite later instructions to use the original source register when that looks safe, and erase trivial self-copy chains.

The file does not own instruction classification or register parsing. It assumes the surrounding peephole pipeline has already converted raw text into `LineInfo` records and that helper APIs can answer whether a line is a barrier, which register family it references, whether it uses implicit architectural registers, and how a rewritten line should be reclassified.

## Important APIs and contract surfaces

The pass surface is a single mutating entry point:

```cpp
bool propagate_register_copies(LineStore* store, LineInfo* infos);
```

Input contract:
- `store` and `infos` are parallel arrays over the same line sequence.
- `infos[i]` must already be classified from `store->get(i)`.
- Rewrites must keep those arrays synchronized by reclassifying changed text.

The file relies on these external helpers instead of parsing assembly itself:

```cpp
LineInfo classify_line(std::string_view raw);
std::optional<std::pair<RegId, RegId>> parse_reg_to_reg_movq(const LineInfo&, std::string_view);
bool has_implicit_reg_usage(std::string_view trimmed);
bool is_shift_or_rotate(std::string_view trimmed);
RegId get_dest_reg(const LineInfo& info);
std::string replace_reg_family(std::string_view line, RegId old_id, RegId new_id);
std::string replace_reg_family_in_source(std::string_view line, RegId old_id, RegId new_id);
```

Local helper surfaces are implementation-only:
- `mark_nop`: converts a line record into a semantic nop without rewriting text.
- `replace_line`: updates text in `LineStore` and immediately re-runs `classify_line`.
- `trimmed_line`: views the line after `LineInfo::trim_start`.
- `try_propagate_into`: one-line safety gate plus rewrite helper for substituting an aliased register use.

## Dependency direction and hidden inputs

Dependency direction is inward toward `peephole.hpp`. This file depends on:
- `LineInfo` classification semantics, especially `is_barrier()`, `is_nop()`, `dest_reg`, `reg_refs`, and `has_indirect_mem`.
- The repo-wide `RegId` numbering for the 16 GP registers.
- The exact textual replacement behavior of `replace_reg_family*`.

Hidden inputs that materially shape behavior:
- `reg_refs` is a precomputed bitset; propagation decisions trust it instead of rescanning text.
- `trim_start` controls what part of each stored line is considered instruction text.
- `has_implicit_reg_usage` is the main "unknown side effects" brake; when it says yes, the pass abandons active copy knowledge.
- `parse_reg_to_reg_movq` defines what counts as a copy source. Anything outside that recognizer is invisible to this pass.

## Responsibility buckets

### 1. Alias tracking

The pass keeps a fixed `copy_src[16]` table mapping destination GP registers to their current known source register. It normalizes through one level of chaining, so a newly seen `movq %a, %b` can become `movq %root, %b` if `%a` is already known to copy `%root`.

### 2. Rewrite of copy instructions

When the current line is a recognized register-to-register `movq`:
- collapse a chain into a direct copy from the ultimate source
- convert `movq %r, %r` into a nop-like record
- invalidate aliases that pointed at the overwritten destination

This is the pass's clearest fast path and also its most literal optimization.

### 3. Forward substitution into later instructions

For non-copy lines, the pass scans active aliases and tries to replace a referenced destination register with its source register. The rewrite is blocked when:
- the line uses indirect memory
- the line has implicit register usage
- `%rcx` is involved in shift/rotate instructions
- the line writes the source register that would be substituted in

If the destination register is also the line's explicit destination, the rewrite uses a source-only replacement helper rather than replacing every occurrence.

### 4. Alias invalidation

Knowledge is discarded aggressively on:
- barriers such as labels, control-flow instructions, calls, returns, and directives
- any line marked with indirect memory access
- explicit writes to a GP destination register
- any line with implicit register usage

The file prefers losing optimization opportunities over carrying alias state across unclear effects.

## Notable fast paths, compatibility paths, and overfit pressure

### Optional fast paths

- Direct chain collapse of consecutive register copies.
- Self-copy elimination by marking the line info as `Nop`.
- Single-line forward substitution when helper predicates say the instruction is simple enough.

### Legacy compatibility paths

- The pass is text-driven even after classification; it rewrites stored assembly strings and reclassifies them.
- It hardcodes 16 x86-64 GP register names in `kReg64Names` to rebuild collapsed `movq` text.
- It treats indirect memory and implicit register usage as coarse compatibility fences rather than modeling exact effects.

### Overfit pressure

The pass is vulnerable to becoming a pile of shape filters because safety mostly comes from recognizers like `parse_reg_to_reg_movq`, `has_implicit_reg_usage`, and `is_shift_or_rotate`. Rebuild work should resist adding more one-off textual exclusions for named instruction forms. The right direction is a clearer instruction-effect model, not a longer denylist of "do not propagate here" cases.

## Rebuild ownership

This file should own: local copy-alias propagation policy for already-classified x86 peephole instructions, including when alias knowledge is created, applied, and invalidated.

This file should not own: general instruction parsing, full architectural side-effect modeling, register taxonomy, line storage/classification mechanics, or ad hoc compatibility rules for individual textual instruction shapes beyond a well-defined effect contract.
