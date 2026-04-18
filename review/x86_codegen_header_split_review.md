# X86 Codegen Header Split Review

- Review base: `51ba6051` (`[plan] switch active runbook to local-memory family`)
- Why this base: it is the current activation/reset checkpoint for the active `ideas/open/54_x86_backend_c_testsuite_capability_families.md` runbook, and later history does not contain another `plan.md` checkpoint for this same route.
- Commit count since base: `2`

## Findings

### Medium: `x86_codegen.hpp` is carrying backend implementation, not just shared contract, and is now large enough that the ownership boundary is no longer defensible

`src/backend/mir/x86/codegen/x86_codegen.hpp` still advertises itself as a transitional surface for sibling translation units, which is a reasonable header role, but the file also contains the full prepared-module renderer body as an inline implementation starting at `emit_prepared_module(...)` and continuing to the end of the file. That implementation owns target validation, lane selection, local-slot layout, same-module global/data emission, control-flow pattern recognition, and final asm text construction in one header-defined function. See [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:35), [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1031), and [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:3755).

This is no longer just "shared API plus types". It is a monolithic implementation blob embedded in a header that every sibling x86 codegen translation unit includes. That makes compile cost, locality, and future ownership worse each time a new prepared-module lane lands.

### Medium: splitting is justified, but not as the current active packet under the local-memory runbook

The current active plan is explicitly about the local-memory semantic family and names `lir_to_bir_memory.cpp` as the primary implementation surface, with the x86 prepared-module handoff kept as a downstream boundary/proof surface. See [plan.md](/workspaces/c4c/plan.md:10), [plan.md](/workspaces/c4c/plan.md:88), and [plan.md](/workspaces/c4c/plan.md:136). Since the review base, the actual code diff only touches [src/backend/bir/lir_to_bir_memory.cpp](/workspaces/c4c/src/backend/bir/lir_to_bir_memory.cpp:1523); there is no new churn in `x86_codegen.hpp`.

So: the split is the right architectural move, but it is not the right thing to silently fold into the currently active local-memory packet. If the team wants to do it now, it should be a deliberate route change or a separate idea/runbook, not an opportunistic side quest inside Step 3 semantic lowering.

## Judgments

- Plan alignment: `on track`
- Idea alignment: `matches source idea`
- Technical-debt judgment: `action needed`
- Validation sufficiency: `narrow proof sufficient`
- Reviewer recommendation: `pause and discuss route change`

## Recommended Split Seams

The safest split is to preserve the existing public handoff contract in the header and move only implementation out.

### Keep in `x86_codegen.hpp`

- `X86Codegen` state/types and shared declarations used by sibling x86 translation units, such as the large translated backend surface around [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:495)
- the public declarations for:
  - `emit_prepared_module(...)`
  - `emit_module(const bir::Module&)`
  - `emit_module(const lir::LirModule&)`
  - `assemble_module(...)`
- tiny inline helpers only if they are genuinely header-only and broadly shared

### First extraction: move the prepared-module renderer into one `.cpp`

Create a focused implementation unit such as `src/backend/mir/x86/codegen/prepared_module_emit.cpp` and move the full body of `emit_prepared_module(...)` there.

This seam is defensible because the function is already a self-contained entrypoint with no callers depending on its internals; tests and `emit.cpp` only need the declaration. See [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp:4) and [src/backend/mir/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/emit.cpp:36).

### Inside that new `.cpp`, split by ownership rather than by testcase lane

1. `prepared_module_emit_support.*`
Keep target/profile validation, block lookup, local-slot layout, asm symbol naming, string/global-data helpers, and other neutral support routines here. These are the reusable mechanics under the handoff contract, not individual capability lanes. Natural candidates live near [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1033), [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1110), and [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1253).

2. `prepared_module_multi_function_lane.*`
Own the bounded multi-defined-function/main-entry lane, including trivial-defined-function rendering and its same-module string/global dependencies. This is a coherent prepared-module admission family around [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1412) and [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1441).

3. `prepared_module_local_memory_lane.*`
Own the single-function local-slot/local-address/local-guard renderers, including:
`try_render_minimal_local_slot_return`, `try_render_local_slot_guard_chain`, `try_render_local_i32_arithmetic_guard`, and `try_render_local_i16_arithmetic_guard`. These all share local-slot layout and stack-addressing semantics and belong together. Their cluster starts around [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1706), [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1816), and [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:2561).

4. `prepared_module_control_flow_lane.*`
Own the compare/join/short-circuit branch families that are about bounded control-flow legality rather than local-memory itself. This includes the recursive block renderer and materialized compare join path around [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:1856), [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:2326), and [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:3272).

5. `prepared_module_direct_call_lane.*`
Own the direct extern-call / constant-folded single-function lane around [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp:3444). This is a separate admission family with its own support logic and should not stay tangled with local-memory or compare-join code.

## Route Advice

If the team acts on this now, the most honest route is:

1. Create a new idea or explicit plan reset for "x86 prepared-module renderer de-headerization".
2. First move `emit_prepared_module(...)` out of the header without changing behavior.
3. Prove equivalence with the current narrow x86 handoff tests before any capability-family work resumes.
4. Only after that, decide whether further sub-file splits are worth doing immediately.

What I would not recommend is mixing this refactor into the active local-memory semantic packet. The current runbook is still about honest `lir_to_bir` family growth, and this header split is architectural cleanup around the downstream handoff surface.
