Status: Active
Source Idea Path: ideas/open/572_rv64_same_module_call_result_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Same-Module Call Representation

# Current Packet

## Just Finished

Completed Step 1 inventory for ordinary RV64 same-module `CallInst`
representation.

Representation findings:

- BIR uses ordinary direct `bir.call` instructions, not inline-asm carriers.
  The failing representatives are `main` block `entry` instruction 0 in
  `src/20000412-2.c` (`%t0 = bir.call i32 f(i32 100, ptr 0)`) and `baz`
  block `entry` instruction 3 in `src/20000622-1.c`
  (`%t4 = bir.call i64 foo(i64 %t1, i64 %t3, i64 %p.b)`).
- Direct callees are carried as `CallInst::callee` text plus prepared
  `PreparedCallPlan::direct_callee_name`; same-module targets have
  `wrapper_kind=same_module`, `is_indirect=false`, and no
  `indirect_callee`.
- Arguments are already represented by `PreparedCallArgumentPlan` entries.
  The supported GPR lane shape is `value_bank=gpr`,
  `destination_register_bank=gpr`, one-lane destination registers `a0`-`a7`,
  and no destination stack area. Immediate integer/null arguments are recorded
  as immediate argument plans; register arguments carry source value/register
  facts; stack-home arguments in the second representative use frame-slot
  sources that must be materialized or loaded into argument registers.
- Prepared value homes already bind ordinary operands and call results. In
  `20000412-2.c`, `main` publishes `%t0` as value id 9 in register `t0`; in
  `20000622-1.c`, `baz` publishes `%t4` as value id 20 in register `t0`.
  Earlier call results may also be stack homes, such as `%t3` in `baz` at
  stack offset 24, and later call arguments can read those homes.
- Call-result metadata is represented by `PreparedCallResultPlan`. The simple
  register-publication shape is `value_bank=gpr`, source storage `register`
  from ABI register `a0`, destination storage `register`, destination bank
  `gpr`, and one-lane destination width. Stack-slot call results are marked as
  late publication in the prepared dump and need a distinct object-route path
  or must remain unsupported until implemented.
- The first object-emission implementation target is the ordinary-call branch
  before the generic unsupported-instruction fallback. The newer text path
  branches in `prepared_function_emit.cpp` to `emit_riscv_simple_call(...)`
  after excluding inline asm; the legacy prepared-object path branches through
  `fragment_for_prepared_instruction(...)` to `fragment_for_prepared_call(...)`
  before `diagnose_unsupported_prepared_instruction_fragment(...)` produces the
  old generic `unsupported_instruction_fragment` diagnostic.
- Value publication should follow the prepared call-result plan and existing
  after-call move-bundle/ABI binding facts. Register-to-register result
  publication is a simple `mv destination, a0` when needed; stack-slot
  destination results need an explicit store to the prepared stack home before
  later users load or pass that value.
- Unsupported call ABI forms should remain fail-closed with a call-specific
  diagnostic: inline asm carrier forms outside the inline-asm path, indirect
  calls for this same-module slice, variadic calls or nonzero variadic FPR
  argument counts, outgoing stack argument areas, memory/sret returns,
  aggregate/byval transports not already handled, floating-point or vector
  argument/result banks, multi-lane results, mismatched/missing prepared call
  plans, and result destinations without complete prepared value-home facts.

## Suggested Next

Execute Step 2 by adding focused backend object-emission tests for ordinary
same-module GPR calls and integer result publication. Cover an immediate/null
argument call with a register result, a multi-GPR call that consumes a prior
call result, and a negative unsupported-call ABI form with a call-specific
diagnostic.

## Watchouts

- This plan is limited to ordinary same-module RV64 call/result lowering.
- Do not treat `llvm.inline_asm` carriers as ordinary calls.
- Do not implement select, floating-point binary, pointer arithmetic,
  prepared-authority, broad ABI, or runtime-comparison work here.
- Do not add filename-specific matching for `src/20000412-2.c` or
  `src/20000622-1.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Keep unsupported ABI forms on precise call-specific diagnostics instead of
  the old generic fallback.
- The current simple-call helper only accepts register-destination GPR results;
  stack-slot result publication is visible in the prepared dumps and should be
  tested or explicitly kept fail-closed before claiming full result
  publication.
- `emit_riscv_simple_call(...)` also allows `DirectExternFixedArity`; the 572
  slice should keep same-module semantics as the proof focus and avoid
  widening into broad external ABI policy.

## Proof

Inventory-only packet. Run:

`git diff --check -- todo.md`

Proof log: `test_after.log`.
