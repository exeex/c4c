Status: Active
Source Idea Path: ideas/open/266_aarch64_atomics_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Atomic Shard And Current Route

# Current Packet

## Just Finished

Completed Step 1 audit of `src/backend/mir/aarch64/codegen/atomics.md`
against the current compiled AArch64 backend route.

Durable current atomics facts to preserve:

- Prepared atomic carriers already exist and flow through
  `PreparedAtomicOperationCarrier` into AArch64 machine records.
- Current selected atomic kinds are load, store, fence, RMW loop, and
  compare-exchange loop, with atomic-specific opcodes and
  `AtomicMemoryAccess` side effects.
- The accepted scalar widths are 1, 2, 4, and 8 bytes.
- Loads support relaxed/acquire/seq-cst and print ordinary or acquire
  load mnemonics by width.
- Stores support relaxed/release/seq-cst and print ordinary or release
  store mnemonics by width.
- RMW supports exchange/add/sub/and/or/xor, requires old-value result mode,
  keeps an exclusive retry loop, and uses reserved MIR scratch registers for
  the computed value and status.
- Compare-exchange supports boolean-success and old-value result modes,
  requires a retry loop plus monitor-clear-on-failure, records success and
  failure orderings separately, and uses a loaded-value scratch register only
  for boolean-result mode.
- Current tests already exercise selected atomic record construction,
  fail-closed rejected carriers, and structured printer output.

Unsupported/deferred facts to keep fail-closed:

- Incomplete prepared carriers, unsupported widths, unsupported orderings,
  unsupported RMW opcodes, unsupported result modes, and missing value/storage
  authority must keep producing explicit diagnostics.
- Atomic load release/acq-rel, atomic store acquire/acq-rel, relaxed fences,
  compare-exchange release/acq-rel failure ordering, unsupported operation
  kinds, and missing scratch/register authority are outside the selected
  subset.
- The current route should not claim signed narrow atomic-load normalization
  or a stronger per-order fence mapping beyond its existing structured facts.

Obsolete reference-only material to reject:

- The legacy `ArmCodegen` entry points, `operand_to_x0`, `store_x0_to`,
  `state.emit`, fixed `x0`-through-`x4` temporary convention, and Rust mirror
  dependency surface are not current prepared-route contracts.
- The old statement that compare-exchange failure ordering is unused is stale;
  the current printer consults failure ordering for acquire-load selection.
- The old full exclusive-loop recipe is reference material only; Step 2 should
  move existing prepared-route facts, not port the legacy surface wholesale.

## Suggested Next

Start Step 2 by creating `src/backend/mir/aarch64/codegen/atomics.cpp` and
`atomics.hpp`, then move the current atomic-family construction/selection
boundary out of `memory.cpp` while preserving the existing dispatch handoff,
diagnostics, fail-closed subset checks, and printer behavior.

## Watchouts

- Concrete routing issue for Step 2: atomic-family record construction,
  validation, diagnostics, and lowering entry points are currently embedded in
  `memory.cpp`/`memory.hpp`; instruction payload definitions remain in
  `instruction.hpp`, dispatch calls `lower_atomic_memory_operations_for_block`,
  and final assembly spelling lives in `machine_printer.cpp`.
- Keep machine-printer instruction spelling behavior unchanged unless the
  supervisor explicitly delegates printer ownership too.
- Do not invent additional prepared facts or port the fixed-register legacy
  surface.
- Keep unsupported atomic forms explicit and do not weaken tests or
  expectations.

## Proof

Not run; delegated proof said no build required for this audit-only packet.
No `test_after.log` was produced.
