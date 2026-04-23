# `mod.cpp`

## Purpose And Current Responsibility

This file is the orchestration layer for the x86 peephole pass set. It owns the end-to-end string pipeline for the pass directory: split whole assembly text into lines, classify each line once into shared metadata, run a fixed sequence of local rewrites for a bounded number of rounds, then render surviving lines back to assembly text.

It is not where individual optimizations live. The real transformation logic is delegated into per-pass functions declared in `peephole.hpp` and implemented elsewhere in `passes/`.

## Important APIs And Contract Surfaces

The only exported behavior implemented here is the pass-directory entrypoint:

```cpp
std::string peephole_optimize(std::string asm_text);
```

The internal helpers define the actual contract this file relies on:

```cpp
LineStore parse_line_store(std::string_view asm_text);
std::vector<LineInfo> classify_lines(const LineStore& store);
std::string render_line_store(const LineStore& store,
                              const std::vector<LineInfo>& infos);
```

And the orchestration surface is the ordered pass list:

```cpp
changed |= eliminate_loop_trampolines(&store, infos.data());
changed |= combined_local_pass(&store, infos.data());
changed |= fold_memory_operands(&store, infos.data());
changed |= propagate_register_copies(&store, infos.data());
changed |= global_store_forwarding(&store, infos.data());
changed |= optimize_tail_calls(&store, infos.data());
changed |= eliminate_dead_reg_moves(&store, infos.data());
changed |= eliminate_dead_stores(&store, infos.data());
changed |= fuse_compare_and_branch(&store, infos.data());
changed |= fuse_movq_ext_truncation(&store, infos.data());
changed |= eliminate_push_pop_pairs(&store, infos.data());
```

Post-loop cleanup is mandatory and not part of the convergence loop:

```cpp
eliminate_unused_callee_saves(&store, infos.data());
compact_frame(&store, infos.data());
```

Current contract assumptions:

- Input and output are raw assembly text, not typed instructions or CFG nodes.
- `LineStore` and `LineInfo[]` must stay index-aligned for the entire run.
- Passes mutate shared text and metadata in place.
- `LineKind::Nop` is the deletion protocol used by rendering.

## Dependency Direction And Hidden Inputs

Dependency direction is downward into the shared peephole substrate:

- this file depends on `../peephole.hpp`
- helper functions here build `LineStore` and `LineInfo`
- every optimization pass consumes the same mutable `store` plus `infos`
- rendering depends on `infos[i].kind` still truthfully describing whether a line survives

Hidden inputs that materially control behavior:

- the meaning of each line kind, barrier, register reference, and extension form lives in `peephole.hpp` support code, not here
- pass safety depends on all pass implementations honoring the shared indexing contract
- textual assembly formatting is part of the API: newline splitting, preserved line order, and final trailing newline behavior all matter
- pass interactions depend on the hardcoded order, not on an explicit dependency graph

## Responsibility Buckets

### 1. Text-to-working-set conversion

Turns the incoming assembly string into a mutable `LineStore`, preserving line order and representing a terminal newline as an extra empty line.

### 2. Initial classification

Computes one `LineInfo` per line before any rewrites. The file assumes later passes can keep using and updating that metadata without re-running whole-file classification.

### 3. Fixed-point style pass scheduling

Runs the local rewrite sequence for up to four rounds, stopping early when no pass reports a change.

### 4. Non-iterative cleanup

Runs callee-save cleanup and frame compaction once after the main rounds, implying these are treated as terminal normalization steps rather than participants in convergence.

### 5. Rendering

Drops lines whose metadata is now `Nop` and reconstructs the final string with normalized newline emission.

## Notable Fast Paths, Compatibility Paths, And Overfit Pressure

### Fast paths

- early exit when a full round reports no changes
- shared up-front classification instead of re-parsing the entire file between passes
- `Nop`-based deletion avoids rebuilding the line vector during optimization

### Compatibility paths

- the file exists to support a legacy string-based assembly optimizer rather than a typed MIR-to-machine-instruction pipeline
- the fixed pass list encodes compatibility with many emitter-shaped assembly patterns accumulated across sibling pass files
- post-pass rendering preserves the coarse textual interface expected by callers

### Overfit pressure

This file concentrates structural overfit pressure even though it contains little pattern matching itself:

- pass ordering is hardcoded and opaque, so new emitter quirks tend to become “add one more pass in the pipeline”
- the four-round budget is heuristic rather than derived from explicit pass dependencies
- every pass shares one mutable text-plus-metadata substrate, which encourages local shape fixes over a stronger intermediate form
- cleanup passes outside the loop can hide ordering debt instead of making pass prerequisites explicit

## Rebuild Ownership

In a rebuild, this file should own only pipeline orchestration: choosing the optimization stage boundary, sequencing well-defined passes, and handling the working-set lifecycle between parse/classify/optimize/render.

It should not own individual x86 rewrite rules, hidden textual compatibility for emitter quirks, broad line-classification semantics, or the long-term public API shape if the subsystem moves away from raw assembly strings.
