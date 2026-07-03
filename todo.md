Status: Active
Source Idea Path: ideas/open/562_prepared_object_data_zero_fill_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Zero-Fill Contract Coverage

# Current Packet

## Just Finished

Completed plan Step 1 inspection for `src/20000412-1.c`.

Current representative proof:

- Allowlist: `build/agent_state/562_step1_zero_fill_boundary.allowlist`
- Log: `build/agent_state/562_step1_zero_fill_boundary.log`
- Result: still fails at the prepared selected object-data contract diagnostic:
  `status=unsupported_but_coherent object_label_id=2 object_size_bytes=1656
  emitted_byte_count=0 zero_fill_byte_count=0`.

Owning boundary:

- Prepared producer: `src/backend/prealloc/object_data.cpp`
  `populate_prepared_object_data_plans(...)` calls `global_initializer_bytes(...)`
  and publishes unsupported coherent data when no materialized initializer bytes
  are available.
- The zero-fill publication branch currently depends on initializer bytes and
  computes `zero_fill = !global.is_constant && bytes_are_all_zero(*bytes)`, so
  it does not publish zero-fill authority for the representative no-explicit-
  initializer `const char * const wordlist[207]` aggregate.
- RV64 consumption receives these prepared facts through
  `rv64_selected_object_data_contract_facts(...)` in
  `src/backend/mir/riscv/codegen/object_emission.cpp` and correctly reports the
  fail-closed prepared contract diagnostic instead of inventing missing
  zero-fill authority.

Boundary classification: still prepared producer-owned. The boundary has not
moved to RV64 consumption.

## Suggested Next

Execute plan Step 2 by adding focused prepared/backend coverage for
`populate_prepared_object_data_plans(...)` that asserts ordinary
no-explicit-initializer global aggregates publish full-extent zero-fill
authority.

Coverage should include the representative semantic shape: a const global
aggregate whose elements are pointer-typed, with no explicit initializer bytes
materialized. Assert semantic prepared object-data facts such as
`requires_zero_fill`, `has_zero_fill`, and `zero_fill_byte_count ==
object_size_bytes`.

Do not match `src/20000412-1.c`, object label id 2, the 1656-byte extent, or
any filename/label/size-specific shortcut.

## Watchouts

- Keep this as prepared object-data producer work unless inspection proves
  selected object data is already coherent before RV64 consumption.
- Do not change RV64 object emission to accept missing prepared data.
- Do not special-case the representative filename, object label id, or
  1656-byte extent.
- Do not touch expectations, unsupported markers, allowlists, or pass/fail
  accounting.
- Existing RV64 object-emission coverage already proves explicit zero
  initializer-elements can reserve BSS storage; the missing target is the
  prepared producer contract for implicit/no-explicit-initializer aggregate
  zero-fill, especially when `global.is_constant` is true.

## Proof

Ran delegated representative proof:

```sh
printf '%s\n' src/20000412-1.c > build/agent_state/562_step1_zero_fill_boundary.allowlist && ALLOWLIST=build/agent_state/562_step1_zero_fill_boundary.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/562_step1_zero_fill_boundary.log 2>&1
```

The command exited 1 as expected for the current failing representative and
wrote the current diagnostic to
`build/agent_state/562_step1_zero_fill_boundary.log`. No root-level
`test_before.log` or `test_after.log` was written.
