# Current Packet

Status: Active
Source Idea Path: ideas/open/221_aarch64_markdown_driven_backend_case_bringup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Backend Cases And Markdown Owners

## Just Finished

Completed plan Step 1, `Audit Backend Cases And Markdown Owners`, as an
inventory-only packet.

Audited case roots and wiring:

- `tests/backend/case` contains flat C fixtures, not per-target subdirectories.
- Current AArch64-specific public-route cases are:
  - `aarch64_return_zero_smoke.c`: enabled when both `CLANG_EXECUTABLE` and
    `aarch64-linux-gnu-as` are found. Owner: `returns.md`, with prologue/entry
    support from `prologue.md` and `BACKEND_ENTRY_CONTRACT.md`. Smoke route:
    `backend_cli_aarch64_asm_external_return_zero_smoke` emits public
    `c4cll --codegen asm --target aarch64-linux-gnu` output, checks `.globl
    main`, `main:`, `mov w0, #0`, and `ret`, assembles with external
    `aarch64-linux-gnu-as`, links with `clang`, and runs only on an AArch64
    host.
  - `aarch64_no_selected_machine_nodes.c`: enabled negative guard. Owner:
    `MACHINE_INSTRUCTION_NODE_CONTRACT.md` and `BACKEND_ENTRY_CONTRACT.md`.
    Smoke route: `backend_cli_aarch64_asm_no_machine_nodes_fails` expects the
    public AArch64 asm route to fail before emitting output when there are no
    selected printable machine nodes.

Audited markdown owner groups for future AArch64 case bring-up:

- `returns.md`: status enabled only for the narrow return-zero smoke above.
  Deferred cases include `return_zero.c`, `return_add.c`,
  `return_add_sub_chain.c`, `pointer_return_*.c`,
  `aggregate_return_pair.c`, and indirect aggregate-return cases. Reason:
  public case enablement needs structured return payload placement, selected
  return-control machine nodes, and printer smoke beyond the current constant
  zero subset.
- `alu.md`: status deferred. Relevant cases include arithmetic return cases,
  `builtin_abs_i32.c`, `builtin_abs_i64.c`, and simple two-argument arithmetic
  fixtures. Reason: scalar ALU records exist as focused C++ coverage, but the
  public AArch64 case route still needs structured selection to printable
  machine nodes instead of fixture-shaped lowering.
- `comparison.md`: status deferred. Relevant cases include `branch_if_*.c` and
  `three_way_phi_merge_post_add_sub.c`. Reason: compare/branch record coverage
  exists, but public AArch64 enablement needs selected compare/branch nodes,
  condition ownership, and terminal printer coverage for each branch family.
- `memory.md`: status deferred. Relevant cases include local/pointer/array
  fixtures, global load/store fixtures, struct member-offset fixtures,
  dynamic-member-array fixtures, and `builtin_memcpy*`/`builtin_memset*`.
  Reason: memory operand and selected load/store records are present in focused
  tests, but public cases need prepared address ownership, selected memory
  nodes, prologue/frame facts where applicable, and printable load/store smoke.
- `calls.md` plus `prologue.md`: status deferred or blocked by carrier gaps for
  aggregate, variadic, byval, HFA, indirect-call, and function-pointer cases.
  Relevant cases include `local_arg_call.c`, `call_helper.c`,
  `function_pointer_param_direct_arg.c`, aggregate call fixtures,
  `variadic_*.c`, `indirect_variadic_*.c`, and `byval_helper_*.c`. Reason:
  AArch64 public case enablement needs prepared call/frame records, ABI
  argument/result carriers, move records, clobber ownership, and selected call
  printer coverage; HFA/variadic/aggregate splitting should remain blocked
  until explicit carriers exist.
- `globals.md`: status deferred. Relevant cases include `global_*`,
  `defined_*`, `extern_*`, `string_literal_char.c`, and named/nested pointer
  global fixtures. Reason: global address materialization, relocation syntax,
  section ownership, and external link/run behavior must be structured before
  public AArch64 case enablement.
- `inline_asm.md`: status deferred for AArch64 public cases. Relevant fixtures
  are `inline_asm_*.c`, currently wired as x86 semantic-BIR route observations.
  Reason: AArch64 inline-asm constraints, clobbers, operand registers, and
  terminal emission need a markdown-owned structured path before target smoke.
- Other AArch64 markdown owners such as `cast_ops.md`, `float_ops.md`,
  `f128.md`, `i128_ops.md`, `atomics.md`, `intrinsics.md`, and `variadic.md`
  are relevant to subsets of the generic case tree but are not first-pass
  public AArch64 candidates until their carrier and selected-node routes are
  explicit.

Current CMake/test wiring facts:

- `tests/backend/CMakeLists.txt` defines `BACKEND_CASE_ROOT` as
  `tests/backend/case`.
- `find_program(AARCH64_AS_EXECUTABLE NAMES aarch64-linux-gnu-as)` gates the
  positive external AArch64 smoke.
- Generic `c4c_add_backend_codegen_route_test` invokes
  `run_backend_codegen_route_case.cmake` through public `c4cll --codegen asm`.
- Generic `c4c_add_backend_runtime_case` is clang-backed runtime comparison
  coverage and is currently x86-labeled for the wired backend cases.
- The active AArch64 public smoke entry points are
  `run_backend_codegen_failure_case.cmake` for the no-selected-nodes negative
  guard and `run_backend_aarch64_external_smoke.cmake` for return-zero external
  assembler/compiler validation.
- Many `tests/backend/case` fixtures are already wired only for x86 or
  semantic-BIR observation. Unwired fixtures are still AArch64-relevant
  candidates but should stay deferred until their markdown owner has a
  structured `.cpp/.hpp -> machine node/operator -> printer smoke` route.

No missing source or ambiguous markdown owner blocked this audit.

## Suggested Next

Execute plan Step 2, `Commit The Bring-Up Matrix`: create the durable matrix
artifact with owner, implementation route, case files, status, smoke
expectation, and deferred or blocked reason. The first clear candidate group is
the scalar return/ALU path, but the matrix should record the full audited case
families before selecting implementation work.

## Watchouts

- Do not touch `ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md`.
- Do not enable cases before the corresponding markdown feature has a structured implementation route.
- Do not weaken expectations or add testcase-shaped backend shortcuts.
- Keep public smoke validation on the c4cll route with external assembler/compiler proof where applicable.
- The delegated proof command searches a nonexistent root `cmake` path and
  therefore relies on its trailing `|| true`; this is acceptable for this
  inventory proof, but a later matrix proof should avoid that noisy path unless
  the supervisor explicitly keeps it.
- `run_backend_aarch64_external_smoke.cmake` compiles the emitted assembly with
  `clang` and assembles with `aarch64-linux-gnu-as`; it does not use the
  internal assembler, encoder, object writer, linker, or binary output route.

## Proof

Delegated audit proof:

```bash
bash -lc 'set -o pipefail; { find tests/backend/case -maxdepth 2 -type f | sort; rg -n "aarch64|backend_case|backend.*case|case/|clang|as|external|unsupported|aarch64_.*case|return_zero|no_selected" tests/backend CMakeLists.txt cmake src/backend/mir/aarch64 -g "*.cmake" -g "CMakeLists.txt" -g "*.cpp" -g "*.hpp" -g "*.md" || true; } > test_after.log 2>&1; test -s test_after.log'
```

Proof log path: `test_after.log`. This is sufficient for Step 1 because the
packet is an inventory and wiring audit only.
