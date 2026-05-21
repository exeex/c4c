Status: Active
Source Idea Path: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Check Adjacent Pointer Cases

# Current Packet

## Just Finished

Step 4 checked the adjacent pointer cases after the Step 2/3 repairs.

The core idea-355 representatives remain green in the delegated subset:
`00020`, `00103`, `00170`, and `00189` all passed, along with the focused
backend unit/CLI contracts.

Adjacent-case classification:

- `00005` remains a frontend/semantic-BIR admission failure, not an AArch64
  publication residual. The runner reports `FRONTEND_FAIL` with semantic
  `lir_to_bir` failing in `main` under the store local-memory semantic family.
- `00173` remains a timeout and needs a separate lifecycle owner. Read-only BIR
  evidence shows the first bad fact before AArch64 lowering: pointer-derived
  string loads such as `*b` and `*src` are already represented as fixed
  `bir.load_global i8 @.str0` / `@.str0+1` accesses, so the generated loop tests
  the first string byte repeatedly instead of the incremented pointer.
- `00181` remains `RUNTIME_NONZERO` and needs a separate lifecycle owner.
  Prepared call plans place `Hanoi` pointer formals in callee-saved homes
  (`%p.source` in `x20`, `%p.dest` in `x21`), but generated AArch64 initializes
  those homes only on the base-case path before `Move`. The recursive path then
  passes uninitialized `x20`/`x21` into recursive `Hanoi`/`Move` calls. That is
  same-module pointer-formal/home publication, not the address-valued memory or
  external call-argument boundary repaired by this owner.

## Suggested Next

Proceed to Step 5 broader guard and lifecycle decision for idea 355. The
classification evidence supports closing or parking this owner with `00020`,
`00103`, `00170`, and `00189` covered, while splitting `00173` and `00181` into
separate semantic owners if the supervisor wants to pursue them next.

## Watchouts

- Do not edit expectations, unsupported classifications, runners, timeout
  policy, CTest registration, or proof-log behavior.
- Do not special-case c-testsuite filenames, source function names, stack
  offsets, symbol names, or emitted instruction neighborhoods.
- `00170` and `00189` now pass through semantic call-boundary publication; do
  not regress them by reintroducing preserved stack reloads for address
  arguments or late publication from reused scratch registers.
- Keep `00005` out of backend publication work until semantic `lir_to_bir`
  admits it.
- Keep `00173` out of AArch64 publication work until semantic BIR preserves
  dynamic pointer-derived string loads instead of fixed global-byte loads.
- Keep `00181` out of this owner unless lifecycle explicitly widens to
  same-module pointer-formal/callee-saved-home publication.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00020_c|c_testsuite_aarch64_backend_src_00103_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c|c_testsuite_aarch64_backend_src_00005_c|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00181_c)$' | tee test_after.log
```

Result: 8/11 passed. The backend unit/CLI contracts passed, and
`c_testsuite_aarch64_backend_src_00020_c`,
`c_testsuite_aarch64_backend_src_00103_c`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c` all passed. The adjacent residuals
were `00005` (`FRONTEND_FAIL`), `00173` (`Timeout`), and `00181`
(`RUNTIME_NONZERO` segmentation fault).

Proof log: `test_after.log`.
