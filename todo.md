Status: Active
Source Idea Path: ideas/open/441_terminator_select_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Terminator/Select Publication Contract

# Current Packet

## Just Finished

Completed Step 2: defined the first terminator/select publication contract,
narrowed to fused pointer `eq/ne` branch publication, with supporting notes
under `build/agent_state/441_step2_terminator_select_contract/`.

Accepted fused pointer branch facts:

- A prepared conditional branch has a matching prepared
  `branch_condition ... kind=fused_compare` record for the same block.
- The prepared branch condition names the same condition value consumed by the
  terminator.
- The fused compare predicate is pointer equality or inequality only: `Eq` or
  `Ne`.
- Both compare operands are pointer-typed and have authoritative
  GPR-compatible prepared homes, or an explicitly authorized pointer/null
  immediate form.
- The condition value is the legalized `I32` branch condition and has a coherent
  prepared home.
- True/false successor labels are prepared/stable and match the terminator's
  targets.
- RV64 consumption must use the prepared branch condition plus prepared homes as
  authority; raw `bir.ne`/`bir.eq`, raw `bir.cond_br`, block order, testcase
  names, or dump layout are insufficient.

Representative accepted shape:

| Row | Prepared fact | Homes |
| --- | --- | --- |
| `20041112-1 bar.entry`, `%t6 = bir.ne ptr %t2, %t5` feeding `bir.cond_br i32 %t6, block_4, block_5` | `branch_condition entry kind=fused_compare condition=%t6 compare=ne ptr %t2, %t5` | `%t2` register `t0`, `%t5` register `s1`, `%t6` register `s2` |

Rejected adjacent shapes:

- pointer relational predicates such as `ult`, `ule`, `ugt`, `uge`, or signed
  variants until a target predicate policy is explicitly selected;
- select-result publication into branch conditions, including
  `root_is_select=yes` materializations;
- stack-slot select publications and unsupported destination storage;
- missing prepared branch-condition records;
- missing or incoherent compare operand/condition homes;
- prepared target-label mismatch;
- direct-global return authority, completed by idea 440;
- direct-global select-chain facts unless a later contract ties them to a
  terminator/select publication consumer;
- pointer-value memory, local store-source publication, global memory
  publication, expectation/allowlist changes, and pass/fail accounting.

## Suggested Next

Execute Step 3: implement or route the fused pointer `eq/ne` branch publication
packet. Suggested owned files/tests:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/441_step3_fused_pointer_branch_publication/*`

Exact Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Keep this plan limited to terminator/select publication authority.
- Do not reopen direct-global return authority; closed idea 440 routes only the
  remaining `bar.entry` fused pointer compare branch here.
- Do not infer branch conditions, select results, compare operands, or
  terminator homes from raw BIR shape, block order, filenames, function names,
  or one dump layout.
- Keep Step 3 to fused pointer `eq/ne` branch publication unless the supervisor
  explicitly changes the packet.
- Do not include pointer relational predicates (`ult/uge/ule`) or select-chain
  materialization in the first implementation packet.
- Keep missing or incoherent publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 contract validation:

```sh
git diff --check
```

Result: passed.
