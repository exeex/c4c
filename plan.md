# Prepared I16 Formal ABI Publication Runbook

Status: Active
Source Idea: ideas/open/403_prepared_i16_formal_abi_publication.md
Reopened from: ideas/closed/403_prepared_i16_formal_abi_publication.md
Supersedes active runbook from: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md

## Purpose

Repair the producer-side prepared ABI/home metadata gap where callee `i16`
formals in argument registers publish a physical register but no GPR bank.

## Goal

Make direct-call `I16` formals publish target-consumable prepared register-bank
facts before RV64 object emission consumes them.

## Core Rule

Fix the producer-side formal ABI/home publication path. Do not teach RV64
`object_emission.cpp` or scalar instruction lowering to infer a GPR bank from
formal order, type width, or a bankless physical register name.

## Read First

- `ideas/open/403_prepared_i16_formal_abi_publication.md`
- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/closed/407_prepared_i16_same_module_call_arg_abi_publication.md`
- `build/agent_state/395_step1_reclassify_after407_dumps/divmod-1.prepared.txt`
- `build/agent_state/395_step1_reclassify_after407_probe.log`
- `tests/c/external/gcc_torture/src/divmod-1.c`
- `src/backend/prealloc/legalize.cpp` and the producer path that publishes
  direct formal ABI/home facts

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/divmod-1.c`.
- Live failing shape: `bir.sext i16 %p.x to i32` in `div2` reaches RV64 object
  emission with `%p.x` published as `encoding=register bank=none reg=a0`.
- Non-targets: corrected 407 caller-side call-argument facts remain complete;
  `src/20000223-1.c` passes the current representative probe.

## Non-Goals

- Do not implement 395 RV64 opcode lowering while the `I16` formal bank fact is
  incomplete.
- Do not infer GPR bank in RV64 object emission from `reg=a0`, parameter order,
  formal type, or callee ABI convention.
- Do not reopen corrected 407 unless fresh prepared dumps again show the old
  same-module `i16` caller-side producer regression.
- Do not redesign aggregate, byval, sret, variadic, or stack-passed argument
  handling.
- Do not rewrite gcc_torture expectations, change allowlists, or add
  filename-specific handling for `divmod-1.c`.

## Working Model

Previous 403 work repaired one `I16` formal ABI publication surface, but the
current `divmod-1.c` proof exposes another same producer contract failure: a
callee `i16` formal is already assigned to argument register `a0`, yet its
prepared storage has `bank=none`. The RV64 scalar `SExt` lowering path exists,
but it correctly rejects bankless prepared register homes.

The producer must publish explicit GPR bank metadata for direct `I16` formals
that are physically in GPR argument registers. Once this is fixed, `divmod-1.c`
can return to 395 to find the next true RV64 instruction-fragment boundary.

## Execution Rules

- Keep implementation packets scoped to direct/callee `I16` formal ABI or home
  publication.
- Inspect prepared dumps before changing code and record the exact formal
  storage facts in `todo.md`.
- Prefer repairing existing scalar-width handling near the formal ABI/home
  publication path over adding a parallel classifier.
- Preserve existing behavior for `I1`, `I8`, `I32`, `I64`, pointer, aggregate,
  variadic, byval, and stack-passed formals unless local proof requires a
  narrow supporting adjustment.
- Use the supervisor-selected proof command and record exact results in
  `todo.md`.
- Treat diagnostic-only churn, expectation rewrites, RV64 consumer inference,
  and single-case green proof as insufficient progress.

## Step 1: Reconfirm The Bankless I16 Formal Register Gap

Goal: identify the exact producer path and facts that publish callee `I16`
formals with physical argument registers but no GPR bank.

Actions:

- Inspect the fresh `src/divmod-1.c` prepared dump and name each `I16` formal
  with `encoding=register`, physical register, bank, width, and use site.
- Confirm corrected 407 caller-side facts remain complete so the blocker is
  not a call-argument destination publication regression.
- Compare against supported scalar formal widths that publish complete GPR
  bank facts.
- Trace the producer path that creates formal ABI/home storage facts and decide
  the first implementation packet owner.

Completion check:

- `todo.md` names the concrete producer function or helper family to repair,
  the observed bankless `I16` formal facts, contrasting supported facts, and
  the supervisor-delegated proof command.
- If the failure is not this formal ABI/home publication gap, stop and request
  lifecycle review instead of patching RV64 object emission.

## Step 2: Publish GPR Bank Facts For Direct I16 Formals

Goal: repair the producer path so direct `I16` formals in GPR argument
registers publish complete target-consumable register-bank metadata.

Actions:

- Extend the selected producer path for `I16` using the same semantic pattern
  as adjacent supported scalar integer widths.
- Ensure prepared formal storage records include the correct GPR bank when the
  formal is physically in an ABI GPR register.
- Preserve precise diagnostics for unsupported formal shapes that still lack
  legitimate prepared ABI/home facts.
- Add or update focused backend or prepared tests if a local test surface
  already exists for formal ABI publication.

Completion check:

- Fresh `src/divmod-1.c` prepared dump no longer shows `i16` callee formals
  with `encoding=register bank=none reg=aN` as the blocking shape.
- Existing supported scalar formal cases remain supported.
- RV64 object emission does not gain scalar formal ABI inference.

## Step 3: Prove Divmod And Route Residuals

Goal: prove the producer repair removes the `src/divmod-1.c` bankless formal
blocker without crossing the prepared-object consumer boundary.

Actions:

- Run the supervisor-selected build and narrow RV64 gcc_torture backend proof
  for `src/divmod-1.c` and any focused same-family formal tests.
- Inspect fresh prepared dumps for `%p.x`, `%p.y`, and any other `I16` formals
  involved in `div2`, `div4`, `mod2`, and `mod4`.
- Run the supervisor-selected backend CTest subset for the implementation
  slice.
- Classify any newly exposed later failure as inside this idea, back in 395,
  owned by another open idea, or requiring a new producer-side split.

Completion check:

- `todo.md` records exact proof results and states whether this idea is
  complete, needs another `I16` formal publication packet, or should be routed
  to another owner.
- No expectation rewrites, unsupported downgrades, allowlist filtering, or
  filename-specific fixes are used as acceptance evidence.
