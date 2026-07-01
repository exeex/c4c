Status: Active
Source Idea Path: ideas/open/443_rv64_prepared_value_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Prepared Value Consumption

# Current Packet

## Just Finished

Completed `plan.md` Step 3 repair for the object-emission StoreGlobal path.
`fragment_for_prepared_store_global` now consumes `store.value` into `x6/t1`
before appending the PC-relative global address materialization into `x5/t0`,
using the existing fragment append helper so labels and fixups are offset after
the value-preserving move.

Fresh focused evidence for `pr81503.c` now shows the prepared value is preserved
across address materialization:

- Prepared BIR still records `store_source function=foo ... source=%t18` and
  `stored=%t18`.
- Object evidence at
  `build/agent_state/443_step3_repair_prepared_value/pr81503.c4c-objdump.txt`
  shows `mv t1,t0` before the global store address materialization, followed by
  `sw t1,0(t0)`.
- Runtime proof passed with `clang_rc=0` and `c4c_rc=0`.

## Suggested Next

Proceed to Step 4 ordinary-C coverage for the repaired prepared-value
consumption path, using a narrow coverage packet before any broader milestone
validation.

## Watchouts

- The text assembly route still fails with `riscv prepared module emitter does
  not support this prepared global storage layout`, so object-route evidence is
  the useful proof for `pr81503.c`.
- The repair is a general ordering fix in object emission, not a testcase or
  symbol-specific path.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.

## Proof

Ran the delegated proof command exactly. It passed:

- `cmake --build build --target c4cll`
- focused `pr81503.c` prepared-BIR, object emission, objdump, link, and qemu
  comparison with `clang_rc=0` and `c4c_rc=0`
- `git diff --check -- todo.md src/backend/mir/riscv/codegen/object_emission.cpp`
- `ctest --test-dir build -j --output-on-failure -R "^backend_"`

`test_after.log` is preserved. Focused artifacts are under
`build/agent_state/443_step3_repair_prepared_value/`.
