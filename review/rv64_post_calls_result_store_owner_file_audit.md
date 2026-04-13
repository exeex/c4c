# RV64 Post-Calls-Result-Store Owner-File Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Checkpoint Reviewed

- chosen base commit: `0a3c1c3c`
  (`[plan_change] [todo_change] [idea_open_change] lifecycle: block rv64 lane pending post-calls result-store owner-file audit`)
- why this is the right checkpoint:
  it is the latest canonical lifecycle commit for the current active plan, and
  `HEAD` has not advanced past that block-before-routing checkpoint
- commits since base: `0`

## Scope Reviewed

- `plan.md`
- `todo.md`
- `src/backend/riscv/codegen/calls.cpp`
- `src/backend/riscv/codegen/alu.cpp`
- `src/backend/riscv/codegen/globals.cpp`
- `src/backend/riscv/codegen/memory.cpp`
- `src/backend/riscv/codegen/variadic.cpp`
- recent route-audit artifacts under `review/`

## Findings

- Medium: the current branch is aligned with the active runbook, but the
  focused owner families no longer expose another helper-sized follow-on.
  `HEAD` is still sitting on the lifecycle block that explicitly paused the
  lane for this audit, so there is no implementation drift to absorb before
  deciding the next route.
- Medium: `calls.cpp` does not have another bounded next packet left on the
  current shared RV64 surface. The scalar result-store seam is already landed
  in the live owner body, and the file itself now marks the remaining
  untranslated inventory as wider call-lowering work
  ([`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:90),
  [`src/backend/riscv/codegen/calls.cpp`](/workspaces/c4c/src/backend/riscv/codegen/calls.cpp:121)).
- Medium: `alu.cpp` is also exhausted for the current narrow lane. The landed
  unary helpers are the only owner bodies that fit the already-shared operand
  surface, while the remaining integer-binop and i128-copy work still depends
  on broader operand/result state that the shared header does not model yet
  ([`src/backend/riscv/codegen/alu.cpp`](/workspaces/c4c/src/backend/riscv/codegen/alu.cpp:27)).
- Medium: `globals.cpp` has no remaining bounded packet on the present seam.
  `emit_label_addr_impl(...)` is already the single translated owner method in
  the live file, and the remaining global/TLS address bodies still require GOT
  and TLS state that the current RV64 surface does not expose
  ([`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:14),
  [`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:34),
  [`src/backend/riscv/codegen/globals.cpp`](/workspaces/c4c/src/backend/riscv/codegen/globals.cpp:48)).
- Medium: `memory.cpp` remains broader than the helper-sized lane that just
  finished. The live file only reaches concrete C++ through the landed helper
  seams; the remaining visible translated inventory immediately widens into
  default load/store delegation, typed indirect addressing, broader GEP work,
  and shared state queries that are still outside the current header boundary
  ([`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:196),
  [`src/backend/riscv/codegen/memory.cpp`](/workspaces/c4c/src/backend/riscv/codegen/memory.cpp:213)).
- Medium: the obvious parked escape hatch is not in better shape. `variadic.cpp`
  still keeps only local arithmetic helpers while the real owner entries remain
  blocked on broader shared `RiscvCodegen` / state surface, so it is not a
  narrower next packet than the blocked call/global/memory/alu families
  ([`src/backend/riscv/codegen/variadic.cpp`](/workspaces/c4c/src/backend/riscv/codegen/variadic.cpp:30)).

## Judgments

- plan-alignment judgment: `on track`
- technical-debt judgment: `action needed`
- reviewer recommendation: `rewrite plan/todo before more execution`

## Route Decision

Do not issue another implementation packet from the currently focused
call/global/memory/alu lane as-is.

This audit did not find a new helper-sized owner-file packet inside the active
shared RV64 surface. Canonical lifecycle state should be rewritten to record
that:

- the bounded non-`F128`/non-`I128` `calls.cpp::emit_call_store_result_impl(...)`
  seam is complete at the current proof bar
- the currently focused remaining owner families are blocked not by missing
  small declarations, but by a broader shared-state gap
- more execution should stay paused until lifecycle state names a fresh route
  for that broader integration stage instead of pretending another one-method
  packet still exists here

If lifecycle wants to continue this initiative immediately, the next route
should be framed as a new shared-surface expansion audit rather than as another
small translated owner-body packet.

## Rationale

This is a route-selection checkpoint, not a drift event. The active plan told
execution to stop and wait for a fresh owner-file audit, and the repo has done
exactly that. The audit result is that the previously productive helper-sized
lane now appears exhausted: the live `calls.cpp`, `alu.cpp`, `globals.cpp`,
and `memory.cpp` files all reserve their remaining work for surfaces that are
materially wider than the current shared RV64 header. Reissuing another narrow
executor packet from these families would only hide that state-expansion need
inside implementation churn.
