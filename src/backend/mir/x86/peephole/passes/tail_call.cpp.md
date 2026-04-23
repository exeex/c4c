# `tail_call.cpp` extraction

## Purpose and current responsibility

This file implements one peephole pass, `optimize_tail_calls`, that rewrites a
very narrow call-plus-epilogue shape into a jump. It is not a general tail-call
analysis pass. It works on already-emitted x86 assembly text plus per-line
classification metadata and recognizes a specific post-call teardown pattern:

- `call` or `callq`
- later `movq %rbp, %rsp`
- later `pop %rbp`
- later `ret`

When the pattern matches, the pass turns the `call` line into a nop in the
metadata array and overwrites the `ret` line with `jmp ...`.

The file also owns local suppression heuristics meant to avoid unsafe rewrites
when the function body appears to use stack-address-sensitive forms.

## Important APIs and contract surfaces

Primary exported surface:

```cpp
bool optimize_tail_calls(LineStore* store, LineInfo* infos);
```

This pass depends on `LineStore` and `LineInfo` from the peephole framework:

```cpp
struct LineInfo {
  LineKind kind;
  std::uint16_t trim_start;
  RegId dest_reg;
  bool has_indirect_mem;
  std::int32_t rbp_offset;
  std::uint16_t reg_refs;
};

struct LineStore {
  std::size_t len() const;
  const std::string& get(std::size_t i) const;
  std::string& get(std::size_t i);
};
```

Local helper surfaces define the pass shape:

```cpp
void mark_nop(LineInfo& info);
void replace_line(LineStore* store, LineInfo& info, std::size_t index,
                  const std::string& text);
bool is_tail_call_candidate(const LineStore* store, const LineInfo* infos,
                            std::size_t call_idx, std::size_t len,
                            std::size_t* ret_idx);
std::optional<std::string> convert_call_to_jmp(std::string_view trimmed_call);
```

Contract assumptions:

- `infos` must stay index-aligned with `store->lines`.
- overwriting text requires immediate reclassification via `classify_line(...)`
- `trim_start` is trusted as the canonical way to slice indentation away
- `dest_reg == 0` is treated as an unsafe write to `%rax`
- labels not starting with `.L` are treated as function-entry-like boundaries
- `.cfi_startproc` also resets per-function suppression state

## Dependency direction and hidden inputs

Direct dependencies flow inward from the peephole framework:

- textual assembly from `LineStore`
- line categorization from `classify_line`
- string helpers `starts_with` and `ends_with`
- register encoding conventions embedded in `dest_reg`
- line-kind taxonomy in `LineKind`

Hidden inputs and implicit policy:

- The pass depends on classifier precision for `Call`, `Ret`, `Pop`, `LoadRbp`,
  `Directive`, `Label`, and fallback `Other`.
- Safety is approximated through textual matching, not CFG or ABI proof.
- A hardcoded scan window of 30 lines limits how far the epilogue may be from
  the call site.
- The pass uses function-scoped sticky suppression, so one suspicious stack
  pattern disables all later tail-call rewrites in that function.
- Indirect thunk calls with prefix `__x86_indirect_thunk_` are explicitly
  excluded from conversion.

## Responsibility buckets

### 1. Candidate detection

`is_tail_call_candidate(...)` walks forward from a call, skipping nop/empty/
directive lines and accepting only a tight sequence that looks like a canonical
frame teardown ending in `ret`.

### 2. Per-function suppression

The main loop flips `func_suppress_tailcall` when it sees stack-address-related
`lea* ... (%rbp|%rsp)` forms or register-based `subq %..., %rsp`. This is a
coarse guardrail against transforming functions whose stack frame may be used in
ways the pass cannot reason about.

### 3. Call-to-jump textual conversion

`convert_call_to_jmp(...)` only accepts `call`/`callq`, preserves indirect-call
spelling by emitting `jmp *...`, and rejects retpoline thunk calls.

### 4. In-place mutation

Successful rewrites:

- mark the original call metadata as `Nop`
- rewrite the `ret` line to `jmp ...`
- reclassify the rewritten line

This means the pass spreads one logical transformation across two indices.

## Notable fast paths, compatibility paths, and overfit pressure

### Core lowering

- Direct `call`/`callq` followed by explicit frame teardown and `ret`

### Optional fast path

- Skipping over `Nop`, `Empty`, and `Directive` lines while looking for the
  epilogue shape

### Legacy compatibility

- Accepts both `call` and `callq`
- Accepts indirect calls by converting to `jmp *target`
- Uses both non-local labels and `.cfi_startproc` as function-boundary hints

### Overfit pressure

- The pattern match is tightly shaped around `movq %rbp, %rsp; pop %rbp; ret`
  rather than a generalized epilogue model.
- `%rax` write rejection via `dest_reg == 0` is a hidden numeric convention,
  not a self-describing contract.
- The suppression rule keys off literal textual fragments such as `(%rbp)`,
  `(%rsp)`, and `subq %..., %rsp`, which is fragile and testcase-shaped.
- The 30-line search horizon is arbitrary and may reflect observed output
  shapes rather than a principled ownership boundary.
- Retpoline compatibility is embedded as a name-prefix special case.

## Rebuild ownership guidance

A rebuild should own:

- an explicit tail-call eligibility model
- a clear epilogue representation instead of ad hoc textual scanning
- named safety predicates instead of hidden register-number conventions
- separation between function-boundary tracking, candidate analysis, and edit
  application

A rebuild should not own:

- broad stack-safety policy encoded as scattered string matching
- ABI-sensitive assumptions hidden in magic constants
- compatibility decisions mixed directly into candidate recognition
- multi-line mutation logic that depends on reclassifying overwritten raw text
