# RV64 Post-Scalar-Cast Broader Route Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `fe14aff5`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending broader post-scalar-cast route`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the active plan and already
  records that the scalar cast owner seam landed cleanly while pausing the
  lane for a broader route decision
- commits since base: `0`

## Scope Reviewed

- `src/backend/traits.cpp`
- `src/backend/riscv/codegen/cast_ops.cpp`
- `src/backend/riscv/codegen/riscv_codegen.hpp`
- `src/backend/riscv/codegen/i128_ops.cpp`
- `CMakeLists.txt`
- `ref/claudes-c-compiler/src/backend/traits.rs`
- `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`

## Findings

- Medium: the smallest truthful broader follow-on is the shared default-cast /
  i128-cast seam, not a pivot to another parked owner family. After the landed
  scalar cast packet, the Rust RV64 cast owner falls through to the shared
  `emit_cast_default(...)` path for the remaining non-F128 cast families,
  while the current C++ RV64 owner still throws as soon as an i128 path is
  requested
  (`ref/claudes-c-compiler/src/backend/traits.rs`,
  `ref/claudes-c-compiler/src/backend/riscv/codegen/cast_ops.rs`,
  `src/backend/riscv/codegen/cast_ops.cpp`).
- Medium: this broader route is still integration-first, not a full i128-owner
  expansion. The RV64 translated inventory already contains the specific
  primitives the shared default-cast path needs next, including accumulator-
  pair load/store, sign/zero-extend-high, and the compiler-rt float/i128 cast
  helpers, but that surface is still parked in `src/backend/riscv/codegen/i128_ops.cpp`
  instead of being wired into the shared RV64 header/build boundary
  (`src/backend/riscv/codegen/i128_ops.cpp`,
  `CMakeLists.txt`).
- Medium: the shared C++ layer does not yet expose the cast-default seam that
  the Rust route expects. `src/backend/traits.cpp` currently only carries the
  bounded shared return-default helper, so choosing the shared cast-default
  seam next keeps the lane on the same proven pattern instead of inventing a
  fresh RV64-local workaround
  (`src/backend/traits.cpp`,
  `ref/claudes-c-compiler/src/backend/traits.rs`).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `route next lifecycle slice to shared default-cast / i128-cast seam`

## Route Decision

Rewrite `plan.md` / `todo.md` so canonical state selects this broader next
route:

- route the next execution packet to the shared `src/backend/traits.cpp`
  default-cast seam modeled on Rust `emit_cast_default(...)`
- keep the RV64 owner delta minimal:
  wire only the `cast_ops.cpp` delegation path plus the shared
  `riscv_codegen.hpp` / `i128_ops.cpp` hook and build surface needed for
  accumulator-pair load/store, sign/zero extension, and float/i128 compiler-rt
  cast helpers
- keep these families parked:
  broader memory typed-indirect and broader GEP owner work,
  `globals.cpp::emit_tls_global_addr_impl(...)`, broader outgoing-call
  lowering and F128/I128 result-store follow-on work, and the parked
  integer-owner / fused-branch / select comparison families beyond the minimal
  cast helper surface this route proves necessary

## Rationale

The blocked post-scalar-cast checkpoint was correct, but the next truthful
route is visible without guessing. The landed scalar packet already made the
translated non-F128/non-I128 owner body real; the remaining visible cast gap is
the shared default path that the Rust backend uses for i128-related and other
default-cast families. That is broader than another one-method owner packet,
but still materially narrower than pivoting to memory, TLS, calls, or broader
comparison work.

This route also fits the repo's integration pattern. The shared return-default
helper in `traits.cpp` proved that a bounded cross-file seam can be landed
without reopening the whole backend. Extending that shared layer next for
default cast handling, while wiring only the minimum RV64 i128 helper surface
needed to support it, keeps the lane honest and bounded.
