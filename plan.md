# Statement Emitter Compression Runbook

Status: Active
Source Idea: ideas/open/stmt_emitter_compression_plan.md

## Purpose

Refactor [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) into smaller helper families without changing lowering behavior.

## Goal

Compress the statement-emission implementation so high-level dispatch and policy read as small coordinators over named helpers.

## Core Rule

Keep each slice behavior-preserving, test-backed, and narrowly scoped to one helper family at a time.

## Read First

- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)
- [`ideas/open/stmt_emitter_compression_plan.md`](/workspaces/c4c/ideas/open/stmt_emitter_compression_plan.md)

## Scope

- Refactor local helper structure inside [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)
- Extract repeated statement-lowering patterns into named helpers
- Preserve current emitted behavior unless a targeted test proves an intended correction

## Non-Goals

- Moving logic into new files unless a later slice proves it is required
- Opportunistic semantic fixes outside the active refactor slice
- Mixing unrelated cleanup into the same patch

## Working Model

Execute from the shallowest coordination layer downward:

1. statement dispatcher compression
2. terminator and block lifecycle helpers
3. aggregate/member/index/address lowering helpers
4. assignment/store helpers
5. call-lowering helpers

Each step should land as one or more small slices with focused tests and full-suite regression checks.

## Execution Rules

- Start each slice by identifying the exact function cluster being compressed.
- Add or update the narrowest tests that exercise the touched lowering path before implementation.
- Compare against Clang IR when refactoring crosses behavior-sensitive codegen paths.
- Prefer extractions that make control-flow contracts explicit.
- Keep helper extraction local within [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) before considering file moves.
- Add comments only when helper contracts are not obvious from names and call structure.

## Step 1: Compress the Statement Dispatcher

Goal: turn the main statement dispatcher into a short coordinator over statement-kind helpers.

Primary target:
- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)

Actions:
- Identify the deepest `emit_stmt` or equivalent dispatcher path.
- Extract per-statement-kind helpers for control flow, declarations, returns, labels/goto/switch, and asm where the current layout supports it.
- Keep low-level emission routines unchanged unless required to complete the extraction.

Completion check:
- The top-level dispatcher reads as a case table over small helpers.
- Existing statement-lowering tests still pass.

## Step 2: Extract Terminator and Block Lifecycle Helpers

Goal: centralize repeated block-open and terminator-guard logic.

Primary target:
- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)

Actions:
- Locate repeated "current block is still unterminated" checks.
- Extract helpers such as `ensure_open_block(...)`, `set_terminator_if_open(...)`, and `emit_fallthrough_br_if_needed(...)` where they match existing behavior.
- Reuse the helper layer across label, branch, return, switch, and unreachable emission sites.

Completion check:
- Terminator guard boilerplate is materially reduced.
- Branch, switch, and return lowering tests remain stable.

## Step 3: Extract Aggregate and Address-Lowering Helpers

Goal: centralize object-access and address-formation policy.

Primary target:
- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)

Actions:
- Extract helpers around member access, field lookup, array indexing, and address materialization.
- Prefer helper surfaces such as `emit_address_of_expr(...)`, `emit_object_member_ptr(...)`, `emit_struct_field_access(...)`, `emit_array_element_address(...)`, and `emit_load_if_lvalue(...)` when they align with current code.

Completion check:
- Repeated aggregate access branches collapse into named helpers.
- Struct field, array indexing, and lvalue-loading tests remain stable.

## Step 4: Extract Assignment and Store Helpers

Goal: separate assignment kind selection from destination-address logic.

Primary target:
- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)

Actions:
- Factor common assignment flow into helpers such as `emit_store_to_lvalue(...)`, `emit_scalar_assign(...)`, `emit_aggregate_assign(...)`, and `emit_init_from_constant_or_expr(...)` where supported by the current code.
- Keep aggregate-copy behavior unchanged unless a targeted test demonstrates a bug.

Completion check:
- Assignment lowering reads as kind-based coordination over store helpers.
- Assignment and aggregate-copy tests remain stable.

## Step 5: Extract Call-Lowering Helpers

Goal: shorten direct, indirect, and variadic call lowering branches.

Primary target:
- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)

Actions:
- Factor argument preparation, signature resolution, and direct vs indirect call emission into named helpers.
- Prefer helper surfaces such as `prepare_call_args(...)`, `emit_direct_call(...)`, `emit_indirect_call(...)`, `resolve_call_signature(...)`, and `coerce_arg_for_call(...)` when they match existing implementation seams.
- Preserve external declaration registration behavior.

Completion check:
- Call-lowering branches are shorter and explicitly named.
- Function pointer, variadic, builtin-like, and extern-call tests remain stable.

## Validation

- Record a full-suite baseline before implementation work.
- Re-run targeted tests after each slice.
- Rebuild and run the full `ctest` suite before handoff.
- Require monotonic full-suite results relative to the recorded baseline.
