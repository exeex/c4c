Status: Active
Source Idea Path: ideas/open/584_rv64_20000819_runtime_mismatch_after_pointer_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Isolate First Divergence Or Abort Owner

# Current Packet

## Just Finished

Step 2 isolated the first concrete divergence for
`tests/c/external/gcc_torture/src/20000819-1.c` with evidence under
`build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/step2/src_20000819-1.c/`.

- Direct execution confirmed `clang.bin` returns `0` under qemu-riscv64 while
  `c4c.bin` aborts with shell return code `134`, so the mismatch is not only a
  CMake harness artifact.
- First owner: `main`, `entry`, instruction `0`,
  `bir.call void foo(ptr %t2, i32 1)`.
- Prepared BIR records the first argument as a computed global address:
  `call_arg_source index=0 encoding=computed_address source_value=%t2 source_base=@a source_delta=4`.
- Prepared homes/call metadata then treat `%t2` as an already available
  register value: `%t2 value_id=18 kind=register reg=s1`, and the call arg is
  encoded as `source_encoding=register source_reg=s1` to ABI register `a0`.
- C4C object emission for `main` emits `mv a0,s1; li a1,1; call foo`, with no
  `@a + 4` materialization and no relocation for `a` in `.rela.text`.
- Clang materializes the pointer before the call with `auipc a0,... # a`,
  `addi a0,a0,4`, `li a1,1`, then calls `foo`.
- Classification: narrow downstream RV64 object/prepared-data publication
  capability for computed global-address call arguments, not a runtime harness
  issue and not fresh evidence that the pointer-publication repair inside
  `foo` is wrong.

## Suggested Next

Run Step 3: add focused coverage for computed global-address call-argument
publication in the RV64 object route. The test should prove a call argument
whose source is `@global + constant_delta` is materialized into the ABI
argument register before a same-module or external call, without relying on the
`20000819-1.c` filename.

## Watchouts

- The first bad value is passed by `main` before `foo` can execute meaningful
  pointer-loop semantics: `foo` receives stale/unmaterialized `s1` through
  `a0` instead of `&a[1]`.
- `c4c.o` does contain global object `a` in `.data` with bytes
  `02000000 00000000`; the missing piece is code/relocation to materialize
  `@a + 4` for the call argument.
- Avoid repairing this through a representative-specific route; the owner is
  computed-address call-argument publication.

## Proof

Step 2 investigation proof wrote `test_after.log`.

- Runtime comparison used the Step 1 `clang.bin` and `c4c.bin` directly with
  `timeout 20 /usr/bin/qemu-riscv64 -L /usr/riscv64-linux-gnu ...`.
- Object/binary evidence was captured with
  `riscv64-linux-gnu-objdump -dr --no-show-raw-insn`,
  `riscv64-linux-gnu-readelf -Ws`, and `riscv64-linux-gnu-readelf -r`.
- Key artifacts: `owner-evidence.txt`, `commands-and-returns.txt`,
  `c4c-o-objdump-dr.txt`, `c4c-bin-objdump-dr.txt`,
  `clang-bin-objdump-dr.txt`, `clang-run.log`, and `c4c-run.log`.
