Status: Active
Source Idea Path: ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Introduce compiled cast_ops owner shell

# Current Packet

## Just Finished

Step 2 of `plan.md` introduced the compiled AArch64 cast-ops owner shell.

Completed work:
- added `src/backend/mir/aarch64/codegen/cast_ops.hpp` with the AArch64
  codegen namespace and nearby shard includes;
- added `src/backend/mir/aarch64/codegen/cast_ops.cpp` as the compiled owner
  translation unit;
- registered `cast_ops.cpp` in `src/backend/CMakeLists.txt`;
- moved no cast behavior and changed no lowering, record, or printer logic.

## Suggested Next

Next movement packet: move scalar cast record construction into the cast-ops
owner without changing behavior.

Owned files for that packet:
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/CMakeLists.txt`
- `todo.md`

Work:
- move `machine_opcode_from_scalar_cast`,
  `make_scalar_cast_instruction_record`, `make_prepared_scalar_cast_record`,
  and `make_prepared_scalar_cast_instruction_record` from `instruction.cpp`
  into `cast_ops.cpp`;
- keep their declarations exported from the new shard header and include that
  header from existing callers as needed;
- preserve all scalar cast selection behavior and diagnostics byte-for-byte
  where practical.

## Watchouts

- Keep this as behavior-preserving cast-ops ownership redistribution.
- Do not expand scalar cast semantics or weaken test expectations.
- Do not touch other AArch64 markdown shards for this active plan.
- Preserve the F128 helper delegation and I128 bitcast transport routes; they
  are cast-shaped routing today but not the first scalar cast shell move.
- The cast printer currently has both conversion spelling and simple integer
  spelling; move those later as spelling helpers, not in the shell packet.
- `cast_ops.hpp` currently exports no behavior; the next packet should be the
  first actual ownership movement.

## Proof

Proof command run for this packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_.*cast'
```

Result: passed. The canonical proof output is preserved in `test_after.log`.
