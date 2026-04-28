# Step 4 Symbol-Call Acceptance Review

## Scope

Objective: review the dirty Step 4 same-module symbol-call renderer slice on top of `07f92093`.

Focus files:
- `src/backend/mir/x86/module/module.cpp`
- `todo.md`

No clang-tools were used; the raw diff and local test artifacts were sufficient.

## Review Base

Chosen lifecycle base: `f8280a1c [plan] reset step 4 loop-countdown route`.

Why: this is the latest lifecycle-tagged `plan.md` checkpoint that reset the active idea-121 Step 4 route. The requested dirty review target is specifically the unstaged work after current `HEAD` (`07f92093 recover prepared guard chain rendering`), but the reviewer workflow base remains the active Step 4 plan checkpoint.

Commits since base: 2.

History ambiguity: none.

## Findings

1. Blocking: local-slot memory validation is not used as the emitted authority.

   In `append_prepared_symbol_call_local_i32_function`, `render_local_slot_memory` calls `render_prepared_local_slot_statement_memory_operand(...)`, but discards the returned prepared memory operand and recomputes the address from BIR local slots plus `module.stack_layout.frame_size_bytes` (`src/backend/mir/x86/module/module.cpp:3351`, `src/backend/mir/x86/module/module.cpp:3365`, `src/backend/mir/x86/module/module.cpp:3376`, `src/backend/mir/x86/module/module.cpp:3384`). That validates that some prepared access exists for the instruction/result/stored value, but it does not make the emitted operand consume the prepared frame-slot id/offset. A prepared frame-slot drift with the same result/stored ids can still emit the BIR-derived slot. This is missing prepared authority recovery for the local-slot part of the route.

2. Blocking: loaded i32 values ignore their prepared register homes.

   The load path requires a prepared register home, then emits `mov eax, <memory>` and records the value as `"eax"` regardless of `load_home.register_name` (`src/backend/mir/x86/module/module.cpp:3612`, `src/backend/mir/x86/module/module.cpp:3629`, `src/backend/mir/x86/module/module.cpp:3630`). This means the route can pass the current BeforeCall bundle mutation while still ignoring prepared value-home authority for load results. That is the same class of authority mismatch the active plan warns against for scalar local-slot rendering.

3. Blocking route-quality concern: route-shaped text-header switching looks testcase-shaped.

   `module_has_same_module_symbol_call_local_i32_route` detects this same-module symbol-call/local-i32 shape and then `emit` repeats `.intel_syntax noprefix` and `.text` before later functions only when that shape is present (`src/backend/mir/x86/module/module.cpp:5830`, `src/backend/mir/x86/module/module.cpp:5879`, `src/backend/mir/x86/module/module.cpp:5892`). That is output-shape behavior gated by a narrow route detector, not semantic renderer support. It should be removed or replaced by a general module text-section policy before accepting this slice.

4. Positive note: the call-lane authority is moving in the right direction.

   The dirty slice does require consumed call plans, checks same-module indirect callee identity against the prepared indirect callee value, uses prepared BeforeCall argument bundles, uses prepared AfterCall result bundles, and limits string emission to constants actually used by this route (`src/backend/mir/x86/module/module.cpp:3319`, `src/backend/mir/x86/module/module.cpp:3422`, `src/backend/mir/x86/module/module.cpp:3439`, `src/backend/mir/x86/module/module.cpp:3465`, `src/backend/mir/x86/module/module.cpp:3524`, `src/backend/mir/x86/module/module.cpp:3656`). I did not find a broad raw call fallback for the same-module callee path.

## Local-Byval Failure

The remaining red `bounded multi-defined helper same-module local byval call route` failure does not, by itself, block committing a clean bounded symbol-call slice. `todo.md` records it as the next helper-prefix boundary, and `test_after.log` shows the delegated proof advanced past the symbol-call route before failing later at that byval helper prefix.

That later failure cannot rescue the current dirty slice, though: the symbol-call implementation still has the blocking authority and route-shape issues above.

## Judgments

Plan alignment: `drifting`.

Idea alignment: `drifting from source idea`.

Technical-debt judgment: `action needed`.

Validation sufficiency: `needs broader proof`.

Reviewer recommendation: `narrow next packet`.

Do not commit this dirty slice as acceptance-ready yet. Keep the same bounded route, but first make local-slot stores/loads emit from the prepared memory operands, make load result emission follow prepared value homes, remove the route-shaped text-header switch, and add or point to mutation proof covering prepared memory-access/frame-slot drift and load-home drift. After that, the known later local-byval helper-prefix failure can be recorded as the next blocker instead of blocking this symbol-call slice.
