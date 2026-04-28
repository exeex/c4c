# Step 4 i8 local-byval helper-prefix slice review

Review base: `f8280a1c495280f2c820505bf0f7180c451c9952` (`[plan] reset step 4 loop-countdown route`).

Why this base: this is the latest plan-history commit that reset the active idea 121 Step 4 route and rewrote `plan.md`/`todo.md` around the current prepared-control-flow recovery constraints. Later commits since this checkpoint are implementation commits, not plan checkpoints.

Commits since base: 3.

## Findings

### High: helper-prefix renderer is testcase-shaped i8 local-byval growth, not the planned semantic Step 4 route

`src/backend/mir/x86/codegen/x86_codegen.hpp:309` adds `render_prepared_i8_local_byval_helper_function`, and the implementation is bounded to exactly one x86_64, void-return, single-block helper with i8 `LoadLocalInst`/`StoreLocalInst`, direct extern calls, and a one-argument same-module pointer call. It emits concrete textual sequences directly (`BYTE` memory operands, `al`, `lea`, `call`) at `src/backend/mir/x86/codegen/x86_codegen.hpp:377`, `src/backend/mir/x86/codegen/x86_codegen.hpp:487`, and `src/backend/mir/x86/codegen/x86_codegen.hpp:538`.

That is not aligned with the current Step 4 runbook. `plan.md` says Step 4 should recover prepared control-flow rendering and explicitly says not to add or accept standalone scalar local-slot renderers in this step unless they are inseparable from prepared branch/label/parallel-copy proof. This slice is a standalone scalar/helper byval lane with no prepared label, branch, join, or parallel-copy identity consumption.

It also conflicts with the source idea's anti-overfit rule. The source idea accepts scalar renderer recovery, but only through semantic renderer coverage for supported forms and nearby same-feature tests. This helper grows the named helper-prefix path around the currently red i8 local-byval case and proves advancement to the next nearby f32 failure, which is exactly the pattern the plan warns against.

### High: dispatch requires every non-entry helper to fit the new narrow i8 renderer

`render_prepared_bounded_same_module_helper_prefix_if_supported` now iterates all non-entry defined functions and rejects the whole prefix if any helper does not satisfy `render_prepared_i8_local_byval_helper_function` (`src/backend/mir/x86/codegen/x86_codegen.hpp:686`). This makes the public helper-prefix path depend on a narrow byval/i8 renderer instead of a reusable prepared helper-function renderer.

The test harness mirrors the route shape: it isolates helpers named `show` and `arg`, trims `arg` by instruction count, and reports failure by exact instruction index (`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3905`, `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3952`). Those probes are useful diagnostics while debugging, but as acceptance proof they are tied to the fixture layout rather than a general prepared statement/expression contract.

### Medium: byval stack-offset publication is directionally semantic and should be split from the renderer shortcut

The prealloc/regalloc changes publish a stack destination for x86 byval pointer arguments while retaining the ABI register binding (`src/backend/prealloc/regalloc.cpp:1451`, `src/backend/prealloc/regalloc.cpp:1894`) and copy that stack destination into call plans from the BeforeCall bundle (`src/backend/prealloc/prealloc.cpp:431`, `src/backend/prealloc/prealloc.cpp:946`). This is consistent with the plan's producer-publication model: the consumer should read prepared authority instead of hard-coding `[rsp]`.

The issue is packaging and proof, not the concept. The only added direct check asserts that some stack offset is published (`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:3519`). It does not add same-feature negative coverage for missing or drifted byval copy destination, nor does it prove that the consumer rejects invalid prepared stack-destination identity.

### Medium: proof still depends on exact fragments and only advances to the adjacent f32 failure

The accepted i8 helper-prefix check looks for exact output fragments such as `mov BYTE PTR [rsp], al`, `lea rdi, [rsp]`, and `call show` (`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:4015`). The broader route also checks the same fragments through `check_route_contains_fragments` (`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp:4424`).

The recorded proof in `todo.md` says the subset now advances past the i8 local-byval rejection and fails at the f32 local-byval helper route. That is a narrow advancement proof, not evidence that the underlying renderer capability has been generalized. Given the route shape above, this is insufficient for supervisor acceptance.

## Judgments

Plan-alignment judgment: `route reset needed` for the helper-prefix renderer slice.

Idea-alignment judgment: `drifting from source idea`.

Technical-debt judgment: `action needed`.

Validation sufficiency: `needs broader proof`; the current narrow proof is not sufficient because the renderer change is testcase-shaped and lacks negative drift coverage.

Reviewer recommendation: `rewrite plan/todo before more execution`.

## Recommendation details

Do not accept the new `render_prepared_i8_local_byval_helper_function` route as Step 4 progress. Either retire it or rewrite the packet around a reusable prepared helper/local-byval renderer with same-feature positive and negative authority coverage. The producer-side byval stack-offset publication appears salvageable as a separate semantic slice, but it should be proven independently with missing/drifted stack-destination tests before being used to justify more helper-prefix rendering.
