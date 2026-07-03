Status: Active
Source Idea Path: ideas/open/563_rv64_f64_global_memory_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect F64 Global-Memory Boundary

# Current Packet

## Just Finished

Completed plan Step 1, `Inspect F64 Global-Memory Boundary`, for
`ideas/open/563_rv64_f64_global_memory_consumption.md`.

Fresh representative proof for `src/20001121-1.c` still fails at:

```text
unsupported_global_data: RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared global memory accesses
```

Current boundary is RV64 consumer-owned. The diagnostic is emitted from
`src/backend/mir/riscv/codegen/object_emission.cpp::diagnose_unsupported_prepared_instruction_fragment(...)`
for `bir::LoadGlobalInst`, after
`rv64_global_scalar_memory_size_for_type(load->result.type)` rejects the
`double` / `F64` result type. The attempted lowering path is
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp::fragment_for_prepared_load_global(...)`,
which currently uses the same integer/pointer scalar-size gate before checking
prepared global-memory facts and before selecting a destination register.

Prepared facts are present before the RV64 gate:

- BIR/prepared target type fact: `foo` contains
  `bir.load_global double @d`.
- Target storage fact: `%t0` is assigned FPR storage, `ft0`.
- Global-memory access fact: prepared addressing records
  `base=global_symbol result=%t0 symbol=d offset=0 size=8 align=8
  base_plus_offset=yes layout_authority=scalar_layout
  range_verdict=proven_in_bounds`.
- Prepared object-data ownership does not appear to be the first blocker:
  `double d;` is an implicit-zero global, and
  `populate_prepared_object_data_plans(...)` publishes BSS zero-fill facts for
  eligible implicit-zero globals.
- Relocation emission has not occurred yet because
  `build_rv64_prepared_text_object_module_with_diagnostics(...)` admits and
  lowers functions before `append_rv64_prepared_data_objects(...)`; the global
  symbol fact needed for the eventual PC-relative relocation is present.

## Suggested Next

Advance to Step 2 coverage/repair preparation: add focused backend coverage for
a prepared `F64` global load with explicit prepared global-symbol memory access
facts and FPR destination storage. The test should prove RV64 still fails closed
when prepared access facts are absent, and should not fabricate object data,
symbols, relocations, or initializer facts in target lowering.

## Watchouts

- Keep ownership in RV64 prepared global-memory consumption. Current evidence
  does not move ownership back to prepared producer state.
- The integer/pointer global-memory path uses GPR load/store helpers. F64
  global loads need an FPR-aware lane, likely `fld`-style consumption of the
  already-prepared global-symbol base-plus-offset facts.
- Do not mix F128, long-double, stack-frame, FPR callee-saved slot, or prepared
  zero-fill work into this lane.
- Do not special-case `src/20001121-1.c`, diagnostic strings, allowlists,
  expected outputs, unsupported markers, or pass/fail accounting.

## Proof

Inspection-only packet. No `test_after.log` was written.

Commands/logs:

```sh
printf '%s\n' src/20001121-1.c > build/agent_state/563_step1_f64_global_boundary.allowlist && ALLOWLIST=build/agent_state/563_step1_f64_global_boundary.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/563_step1_f64_global_boundary.log 2>&1
build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20001121-1.c > build/agent_state/563_step1_f64_global_boundary.prepared.txt 2> build/agent_state/563_step1_f64_global_boundary.prepared.err
build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20001121-1.c > build/agent_state/563_step1_f64_global_boundary.bir.txt 2> build/agent_state/563_step1_f64_global_boundary.bir.err
```

Representative result: `total=1 passed=0 failed=1`.
