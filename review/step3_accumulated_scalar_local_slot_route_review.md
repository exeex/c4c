# Step 3 Accumulated Scalar Local-Slot Route Review

## Review Base

- Chosen lifecycle base: `fd06c9f3 [plan] rewrite step 4 scalar/control-flow routing`.
- Rationale: this is the latest lifecycle checkpoint on `plan.md`/`todo.md` for idea 121, and the delegated review explicitly asks for the current dirty worktree after this commit.
- Commits since base: 0.
- Dirty scope reviewed: `src/backend/mir/x86/module/module.cpp`, `tests/backend/backend_x86_handoff_boundary_local_i16_guard_test.cpp`, `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp`, `todo.md`, and `test_after.log`. The next failing blocker is in `tests/backend/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp`.

## Findings

### High: The dirty slice is too broad to commit as one Step 3 scalar packet

The current worktree contains a large renderer expansion in `src/backend/mir/x86/module/module.cpp`: shared frame-slot helpers, standalone i32 local-slot return support, i32 local-slot guard support, i16 increment guard support, i16/i64 subtract return support, and the local-slot short-circuit/guard helper (`module.cpp:2830`, `module.cpp:2946`, `module.cpp:3090`, `module.cpp:3239`, `module.cpp:3462`, `module.cpp:3722`). The dispatch chain then runs several local-slot control-flow helpers before the broad `validate_prepared_control_flow_handoff` call and runs the i16/i64 scalar helper after it (`module.cpp:3994`, `module.cpp:4003`, `module.cpp:4004`).

That is more than the focused Step 3 “scalar local-slot i16/i64 subtract return” advance described in `todo.md:11`. The dirty test changes also still include Step 4 short-circuit/control-flow authority assertions (`tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:849`, `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:2442`, `tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:2944`). Keeping that work may be valid, but it is a separate control-flow slice and should not be bundled with a new scalar add-chain/sub/guard-lane chase.

This is not yet a route-reset-level overfit finding, because the helpers do consume prepared frame-slot, value-home, return-ABI, branch-condition, branch-plan, and parallel-copy authority in important places. But as accumulated dirty state it is no longer a coherent Step 3 commit candidate.

### High: The renderer strategy is becoming testcase-shaped around exact instruction sequences

The new scalar helpers are prepared-metadata aware, but they still select support by exact BIR topology and instruction indexes: i32 local-slot return requires exactly two instructions (`module.cpp:2968`), i32 guard requires an entry block with exactly three instructions (`module.cpp:3112`), i16 increment guard requires exactly nine instructions and hard-coded access indexes 0/1/5/6 (`module.cpp:3261`, `module.cpp:3355`), and i16/i64 subtract return requires exactly ten instructions with hard-coded access indexes 0/1/2/3/7/8 (`module.cpp:3484`, `module.cpp:3563`).

The current red blocker is `minimal local-slot add-chain guard route`, whose expected shape stores three i32 locals, loads three slots, performs two adds, and branches on the sum (`tests/backend/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp:61`, `tests/backend/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp:153`, `tests/backend/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp:803`). Continuing by adding another exact add-chain helper would cross into testcase-shaped renderer growth. The next packet must either introduce a genuinely reusable scalar expression/local-slot lowering rule or split/record the add-chain route as unsupported for this packet.

### High: The proof is red, so this slice is not commit-ready

`test_after.log` shows configure and build passed, and `backend_x86_prepared_handoff_label_authority` passed, but `backend_x86_handoff_boundary` fails with:

`minimal local-slot add-chain guard route: x86 prepared-module consumer did not emit the canonical asm`

`todo.md:43` through `todo.md:50` correctly record this as non-acceptance proof. Because the dirty slice changes renderer dispatch and widens supported surfaces, a red handoff boundary proof is a blocker before commit. It should force narrowing/splitting before further execution unless the supervisor explicitly delegates a bounded semantic add/sub guard-lane abstraction.

### Medium: Negative coverage is useful but incomplete for the widened scalar surface

The added i16 guard frame-access drift test mutates raw `slot_name` carriers and requires the emitted asm to remain prepared-frame-slot driven (`tests/backend/backend_x86_handoff_boundary_local_i16_guard_test.cpp:386`). The short-circuit missing-frame-slot negative clears prepared frame slots and expects rejection (`tests/backend/backend_x86_handoff_boundary_short_circuit_test.cpp:849`). These are useful authority tests.

The widened scalar surface still needs negative coverage before acceptance: missing prepared memory access for each used instruction index; drifted or missing frame-slot id; wrong access size, especially the current i16 `size=0` compatibility path (`module.cpp:2861`); load/store access divergence for each scalar family; missing value homes for store/load/cast/binary carriers; return move source and destination drift; branch-condition missing/drifted labels for guard lanes; and add-chain/sub guard lanes proving prepared frame slots and value homes over raw local-slot names. A green add-chain positive alone would not be sufficient.

## Judgments

- Plan-alignment judgment: `drifting`.
- Idea-alignment judgment: `matches source idea`, but the active dirty slice is too broad for one Step 3 packet.
- Technical-debt judgment: `action needed`.
- Validation sufficiency: `needs broader proof`; at minimum the delegated x86 handoff subset must be green after splitting/narrowing, and broader backend validation is still required before lifecycle closure.
- Testcase-overfit judgment: risk is high and rising. The current code is not pure fixture-name matching, but another sequence-specific add-chain helper would be testcase-shaped overfit.

## Recommendation

`rewrite plan/todo before more execution`.

Do not commit the accumulated dirty slice as-is. First split the dirty work into coherent packets:

- keep Step 4 short-circuit/control-flow authority changes separate from Step 3 scalar local-slot work;
- keep the already advanced i16/i64 subtract-return scalar slice separate from the next add-chain/sub/guard-lane problem;
- require the next Step 3 packet to define a reusable scalar expression/local-slot lowering rule, not another hard-coded known fixture sequence.

The current red proof should force split or plan-owner work unless the supervisor deliberately narrows the next packet to a semantic add/sub guard-lane abstraction with the negative coverage listed above.
