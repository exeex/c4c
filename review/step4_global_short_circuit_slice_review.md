# Step 4 Global And Short-Circuit Slice Review

## Review Base

- Chosen lifecycle base: `c77da3fe [plan] route step 4 through prepared producer identity repair`.
- Rationale: `5dbb6a07` activated idea 121, but `c77da3fe` is the later lifecycle checkpoint that rewrote Step 4 around the prior control-flow route review's producer-identity repair requirement. The current packet builds on the already-committed producer/value-location identity slice from `6dfb989b`.
- Commits since base: 1.
- Dirty scope reviewed: `src/backend/mir/x86/module/module.cpp`, `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`, `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`, and `todo.md`.
- Prior reports consulted: `review/step4_current_x86_control_flow_route_review.md` and `review/step4_current_prepared_identity_slice_review.md`.

## Findings

### High: The local-slot short-circuit renderer is too fixture-shaped to accept as Step 4 progress

`src/backend/mir/x86/module/module.cpp:2851` adds `append_prepared_local_slot_short_circuit_or_guard_function`, then installs it into the prepared renderer dispatch at `src/backend/mir/x86/module/module.cpp:3109`. The helper does consume prepared branch conditions, short-circuit join metadata, branch-owned parallel-copy bundles, stack layout, and prepared labels, so it is not a raw-label fallback. The problem is the renderability rule is still a narrow handwritten template: one join transfer, entry plus RHS local-slot loads, two leading entry stores into the same slot, equal immediates, only `Eq`/`Ne`, leaf returns restricted to `0` or `1`, and hard-coded return ordering.

That shape tracks the current minimal local-slot short-circuit/or-guard fixture more than a reusable semantic lowering class. It also returns `false` for most mismatches instead of surfacing which prepared authority is missing, so the route can silently fall through to later renderers until the exact fixture shape is present. Under `plan.md` Step 4 and the source idea's anti-overfit rule, this should be reworked or narrowed before commit unless the supervisor explicitly approves a tactical fixture renderer.

### Medium: Pointer-backed global compare-join rendering validates ownership but does not actually dereference the prepared pointer root

The same-module global arm support at `src/backend/mir/x86/module/module.cpp:2132` is generally aligned with the prior review: it uses the resolved prepared render contract and validates same-module global ownership before rendering direct global loads. The pointer-backed case at `src/backend/mir/x86/module/module.cpp:2149` additionally validates the pointer-root global and emits `mov rax, QWORD PTR [rip + root]` at `src/backend/mir/x86/module/module.cpp:2170`.

However, the selected value is then loaded directly from `return_arm.global->name` at `src/backend/mir/x86/module/module.cpp:2173` rather than through the loaded root. The expected assembly changes in `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp:307` also accept deferred pointer-root data comments instead of `.quad` symbol initializers. Because `rax` is unused, the pointer-root read is only a validation/dead-load marker, not semantic pointer-backed lowering. This is not the same kind of overfit as a name dispatcher, but it should not be treated as complete pointer-backed global renderer recovery; commit only if `todo.md` keeps the data-emitter/pointer-root behavior as a remaining boundary.

### Medium: The branch-condition target exception is narrow but needs negative coverage before expansion

`prepared_short_circuit_metadata_owns_branch_condition_targets` at `src/backend/mir/x86/module/module.cpp:411` lets `validate_prepared_branch_condition` accept a condition whose prepared control-flow block does not satisfy the normal block/target match, if an exact prepared short-circuit branch plan owns the continuation target ids (`src/backend/mir/x86/module/module.cpp:514`). This is materially better than the earlier rejected raw-label bridge exception: it is anchored in prepared short-circuit metadata and exact prepared labels.

Still, it is a validator exception in the same family as the previous Step 4 drift. The dirty tests update positive execution-site coverage at `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:719`, but I did not see new negative coverage for drifted continuation labels or an unrelated condition being accepted through the branch-plan path. Keep it narrow and add negative coverage before using this exception as a foundation for more short-circuit cases.

### Low: The short-circuit execution-site test update is semantic, not an expectation downgrade

The test rename and assertion rewrite in `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:719` switches from demanding a logical `entry -> logic.end.10` critical-edge bundle to checking authoritative branch-owned bundles and their published execution block. That matches the producer contract described in `todo.md` and is not a supported-to-unsupported downgrade.

## Overfit Assessment

This dirty slice is mixed. The direct same-module global compare-join path is mostly semantic prepared value-location renderer recovery. The short-circuit execution-site test clarification is also semantic.

The new local-slot short-circuit renderer is the route concern. It uses prepared records, but its acceptance surface is still a compact fixture template rather than a general prepared control-flow/value-location lowering rule, and the selected proof still fails later on missing prepared block identity for `contract.rhs.bridge`. I would not call the whole slice expectation drift, but the short-circuit renderer should not be expanded or accepted as-is without rework/narrowing.

## Validation

`test_after.log` records a fresh build/test attempt. `backend_x86_prepared_handoff_label_authority` passed, and `backend_x86_handoff_boundary` failed later with:

`canonical prepared-module handoff rejected x86 control-flow label authority: BIR block 'contract.rhs.bridge' has no prepared block id`

That is useful blocker advancement, not acceptance proof. `git diff --check` is clean.

## Judgments

- Plan alignment: `drifting`.
- Idea alignment: `drifting from source idea`.
- Technical debt: `action needed`.
- Validation sufficiency: `needs broader proof`.

## Recommendation

`rewrite plan/todo before more execution`.

Do not continue expanding the dirty short-circuit renderer. Either split/commit only the semantic global compare-join plus short-circuit execution-site clarification after fresh validation, or rework the local-slot short-circuit route so it publishes/preserves the missing bridge identity and lowers a clearly defined semantic class with nearby positive and negative coverage. The current whole dirty slice should not be committed as a coherent acceptance-ready Step 4 packet.
