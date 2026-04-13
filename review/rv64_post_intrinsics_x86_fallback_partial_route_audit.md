# RV64 Post-Intrinsics-X86-Fallback Partial Route Audit

Date: 2026-04-13
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `82ce1bbf`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: advance rv64 intrinsics route past scalar sqrt-fabs packet`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit that names the current active
  x86-only fallback packet in `plan.md` and `todo.md`, and the only execution
  diff on top of it is the just-landed fallback sub-slice at `813bb64f`
- commits since base: `1`

## Validation Observed

- reviewed the implementation delta for
  [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:519)
  and
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3306)
- reviewed proof evidence from `rv64_intrinsics_x86_fallback_build.log` and
  `rv64_intrinsics_x86_fallback_test.log`
- observed that the build log rebuilt the owned intrinsics and shared-util
  test objects and relinked `backend_shared_util_tests`, while the filtered
  intrinsics test log is empty because the targeted run passed without failure
  output

## Findings

- Medium: canonical lifecycle state is now too broad for the landed slice.
  `todo.md` still describes the active packet as the full remaining x86-only
  fallback family inside `emit_intrinsic_rv(...)`, but commit `813bb64f` only
  wires the AES/CLMUL subset
  (`Aesenc128`, `Aesenclast128`, `Aesdec128`, `Aesdeclast128`, `Aesimc128`,
  `Aeskeygenassist128`, and `Pclmulqdq128`) through the zero-destination
  compatibility path:
  [todo.md](/workspaces/c4c/todo.md:53),
  [src/backend/riscv/codegen/intrinsics.cpp](/workspaces/c4c/src/backend/riscv/codegen/intrinsics.cpp:519),
  [tests/backend/backend_shared_util_tests.cpp](/workspaces/c4c/tests/backend/backend_shared_util_tests.cpp:3423).
- Medium: the translated owner route is still live immediately after the
  landed AES/CLMUL subset. The same translated zero-destination fallback block
  continues with adjacent x86-only SSE shift, shuffle, arithmetic, pack/
  unpack, insert/extract, and conversion cases
  (`Pslldqi128` through `Pextrq128`) before the dispatcher exits the current
  block, so the lane is not yet exhausted:
  [src/backend/riscv/codegen/riscv_codegen.hpp](/workspaces/c4c/src/backend/riscv/codegen/riscv_codegen.hpp:185),
  [ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs](/workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs:259).
- Medium: issuing another executor packet without rewriting lifecycle would
  make packet boundaries harder to judge. The current canonical packet says the
  whole fallback family is active, but the actual branch history now contains
  one narrower accepted sub-slice. `plan.md` and `todo.md` need to record that
  narrower checkpoint before more execution so the next packet is explicit
  rather than inferred from chat or commit history:
  [todo.md](/workspaces/c4c/todo.md:55),
  [plan.md](/workspaces/c4c/plan.md:122),
  [review/rv64_post_intrinsics_x86_fallback_partial_route_audit.md](/workspaces/c4c/review/rv64_post_intrinsics_x86_fallback_partial_route_audit.md:1).

## Judgments

- plan-alignment judgment: `drifting`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not issue another execution packet from the stale lifecycle state.

Rewrite `plan.md` / `todo.md` first so canonical state:

- marks commit `813bb64f` plus the focused
  `rv64_intrinsics_x86_fallback_build.log` /
  `rv64_intrinsics_x86_fallback_test.log` proof lane as the completed
  AES/CLMUL fallback sub-slice inside the broader x86-only compatibility lane
- narrows the next execution packet to the adjacent remainder of the same
  translated zero-destination block:
  `Pslldqi128`, `Psrldqi128`, `Psllqi128`, `Psrlqi128`, `Pshufd128`,
  `Loadldi128`, `Paddw128`, `Psubw128`, `Pmulhw128`, `Pmaddwd128`,
  `Pcmpgtw128`, `Pcmpgtb128`, `Paddd128`, `Psubd128`, `Packssdw128`,
  `Packsswb128`, `Packuswb128`, `Punpcklbw128`, `Punpckhbw128`,
  `Punpcklwd128`, `Punpckhwd128`, `Psllwi128`, `Psrlwi128`, `Psrawi128`,
  `Psradi128`, `Pslldi128`, `Psrldi128`, `SetEpi16`, `Pinsrw128`,
  `Pextrw128`, `Pinsrd128`, `Pextrd128`, `Pinsrb128`, `Pextrb128`,
  `Pinsrq128`, `Pextrq128`, `Storeldi128`, `Cvtsi128Si32`,
  `Cvtsi32Si128`, `Cvtsi128Si64`, `Pshuflw128`, and `Pshufhw128`

Keep these families parked:

- broader atomics follow-on
- `peephole.cpp`
- any non-intrinsics follow-on outside the current translated fallback block

## Rationale

The branch did not jump to a different owner family, but it did narrow the
current packet boundary in practice. Commit `813bb64f` is still a truthful
same-file continuation because it wires the first bounded subset of the
translated x86-only fallback block and proves that the zero-destination
compatibility path works through the already-landed destination-pointer
surface. The route is therefore recoverable without a full reset, but
canonical lifecycle state has to acknowledge the narrower accepted checkpoint
before more execution proceeds.
