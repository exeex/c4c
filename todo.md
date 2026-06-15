# Current Packet

Status: Active
Source Idea Path: ideas/open/270_phase_f5_memory_accesses_prepared_only_same_consumer_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Identify consumer, row, and authority gap

## Just Finished

Completed Step 1 by auditing literal public
`PreparedFunctionLookups::memory_accesses` consumers against current x86/riscv
proof surfaces. No supported same-consumer x86 or riscv row was found that
directly reads `PreparedFunctionLookups::memory_accesses`; the packet is
blocked for the requested exact target.

Literal public-field consumers found:

- `src/backend/mir/aarch64/codegen/alu.cpp` reads
  `context.function.prepared_lookups->memory_accesses` in
  `make_prepared_scalar_load_source(...)` and
  `find_prepared_load_local_source_producer(...)`.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` reads
  `lookups.memory_accesses` for helper/oracle coverage only.
- `src/backend/prealloc/prepared_lookups.cpp` publishes the aggregate field.

Current x86/riscv proof-surface gap:

- `src/backend/mir/x86/module/module.cpp` has a useful adjacent selected
  `LoadLocal` Route 3/Route 5 agreement path:
  `render_agreed_route3_load_local_statement_memory_operand(...)` calls
  `find_agreed_route3_load_local_source_memory_access(...)`, but that path
  reads `PreparedEdgePublication::source_memory_access` and
  `PreparedAddressingFunction` lookups, not the public
  `PreparedFunctionLookups::memory_accesses` field.
- `src/backend/mir/riscv/codegen/emit.cpp` accepts
  `PreparedFunctionLookups`, but the searched riscv paths do not directly read
  `lookups->memory_accesses`.

Nearest supported x86 adjacent row, if the supervisor chooses a fixture-support
or scope-adjustment packet:
`make_x86_param_eq_zero_branch_joined_loadlocal_or_sub_then_add_module()` /
function `branch_join_loadlocal_then_add`; true predecessor block `is_zero`
instruction 0 is `LoadLocal` result `zero.loaded` from local slot
`%contract.selected.source.slot`, BIR byte offset 8, size 4, align 4. The
prepared row is the matching `PreparedMemoryAccess` plus corresponding prepared
edge-publication source-memory fields, but it is not an exact
`PreparedFunctionLookups::memory_accesses` consumer row.

Nearest exact public `memory_accesses` row outside the requested x86/riscv
surface:
`tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`
`scalar_consumers_use_load_local_source_for_unpublished_stack_or_gp_register_results()`;
function `f.load_local`, entry instruction 0 `LoadLocal` result
`%loaded.stack`, local slot `%lv.stack`, prepared frame slot id 1, byte offset
16, size 4, align 4, consumed by AArch64 scalar ALU lowering through
`context.function.prepared_lookups->memory_accesses`. That row is outside the
current x86/riscv target unless the supervisor widens the plan.

Compatibility output to preserve if the x86 adjacent row is selected later:

```asm
.intel_syntax noprefix
.text
.globl branch_join_loadlocal_then_add
.type branch_join_loadlocal_then_add, @function
branch_join_loadlocal_then_add:
    test edi, edi
    jne .Lbranch_join_loadlocal_then_add_is_nonzero
    sub rsp, 4
    mov eax, DWORD PTR [rsp + 16]
    add eax, 2
    add rsp, 4
    ret
.Lbranch_join_loadlocal_then_add_is_nonzero:
    mov eax, edi
    sub eax, 1
    add eax, 2
    ret
```

Blocker map:
to prove the exact source idea without hand-built stale prepared state, add or
expose a supported x86/riscv same-consumer fixture that reaches a backend
consumer reading `PreparedFunctionLookups::memory_accesses`, or ask the plan
owner/supervisor to widen the selected target to the existing AArch64 scalar ALU
consumer.

## Suggested Next

Do not implement Step 2 against the x86 adjacent row as if it proved the exact
public `memory_accesses` target. Supervisor should choose one of two next
packets: create narrower fixture support for an x86/riscv public
`PreparedFunctionLookups::memory_accesses` consumer, or delegate a plan-owner
scope adjustment to use the existing AArch64 scalar ALU public-field consumer.

## Watchouts

- Treat the x86 selected-`LoadLocal` path as adjacent fixture evidence only; it
  currently proves prepared edge-publication/source-memory agreement, not direct
  public `PreparedFunctionLookups::memory_accesses` consumption.
- The stable route output is generated from ABI helpers, but for the x86_64
  fixture today it is the `eax`/`edi` assembly listed above.
- Helper/oracle checks are useful support, but helper-level
  `lookups.memory_accesses` coverage alone is not sufficient.
- Do not use hand-built stale prepared state as proof of supported same-consumer
  coverage.
- Do not hide, demote, delete, privatize, wrap, or rename away
  `memory_accesses`.
- Do not weaken helper/oracle statuses, fallback names, unsupported behavior,
  route-debug output, prepared-printer output, wrapper output, exact target
  output, or baselines.
- Helper-level proof alone is not sufficient if it does not prove the selected
  target consumer fails closed.

## Proof

Read-only/todo-only packet. No build or test proof was required, and no
`test_after.log` was produced.

Recommended next proof command:

```sh
cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$'
```

Use the x86 command only for an x86 fixture-support packet. If the supervisor
widens to the existing AArch64 public-field consumer, use:

```sh
cmake --build --preset default --target backend_aarch64_prepared_scalar_alu_records_test && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_prepared_scalar_alu_records$'
```
