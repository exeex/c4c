Status: Active
Source Idea Path: ideas/open/438_prepared_pointer_value_memory_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Prepared Authority Contract

# Current Packet

## Just Finished

Completed Step 1: audited pointer-value memory evidence and existing prepared
authority for idea 438.

Representative row: `930930-1`.

Current object result:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Pointer-value access bucket table:

| access | source pointer value | pointer home | access | current authority | first missing fact |
| --- | --- | --- | --- | --- | --- |
| `f:block_4:inst1` | `%t5`, loaded from `%lv.param.reg1` | `%t5` stack slot `slot#12+stack96` | load `%t6`, `size=8 align=8 offset=0` | `base=pointer_value`, `base_plus_offset=yes`, `layout_authority=unknown`, `range_verdict=unknown_compatible` | Pointee object/layout authority for runtime `reg1`; range cannot be proven in bounds. |
| `f:logic.rhs.11:inst1` | `%t15`, loaded from `%lv.param.reg1` | `%t15` stack slot `slot#17+stack128` | load `%t16`, `size=8 align=8 offset=0` | `base=pointer_value`, `base_plus_offset=yes`, `layout_authority=unknown`, `range_verdict=unknown_compatible` | Same missing runtime `reg1` pointee authority. |
| `f:block_5:inst1` | `%t24`, loaded from `%lv.param.reg1` | `%t24` stack slot `slot#22+stack160` | load `%t25`, `size=8 align=8 offset=0` | `base=pointer_value`, `base_plus_offset=yes`, `layout_authority=unknown`, `range_verdict=unknown_compatible` | Same missing runtime `reg1` pointee authority; feeds the later indirect store value. |
| `f:block_5:inst5` | `%t27`, computed from `%t26 - 8`; `%t26` loads `%lv.param.mr_TR` | `%t27` stack slot `slot#25+stack184` | store `%t25`, `size=8 align=8 offset=0` | `base=pointer_value`, `base_plus_offset=yes`, `layout_authority=unknown`, `range_verdict=unknown_compatible` | Authority that decremented runtime `mr_TR` still points inside the intended object/range. |

Related pointer homes:

- `%t0`, `%t26`, `%t30`, and `%t31` publish `pointer_base_plus_offset` homes,
  but those homes do not prove pointee layout/range for the four
  pointer-value memory records.
- `%t26` feeds `%t27`, but the store still has only unknown pointer-value memory
  authority after the decrement.

Existing authority surface:

- BIR/provenance code has a route for stronger pointer-value provenance, but
  the representative records remain `layout_authority=unknown` and
  `range_verdict=unknown_compatible`.
- Stack-layout publishes pointer-value access records with base value, size,
  alignment, offset, and base-plus-offset shape.
- Existing BIR tests prove pointer-value access record publication, and
  existing RV64 tests prove manually-authored pointer-value accesses can lower,
  but they do not establish or require producer layout/range authority.

Disposition: this is producer/prepared authority work first. Do not select an
RV64 target-consumer packet until the prepared contract can distinguish coherent
pointer-value memory from unknown runtime pointer dereferences.

Artifacts:

- `build/agent_state/438_step1_pointer_value_memory_audit/audit.md`

## Suggested Next

Execute Step 2: Define The Prepared Authority Contract.

Recommended packet:

- Define the minimum prepared facts for pointer-value memory ownership, layout
  authority, range verdict, and base-plus-offset validity.
- Add or identify focused producer/contract tests proving unknown
  pointer-value authority stays fail-closed and coherent explicit authority is
  accepted if existing fixtures can express it.
- Defer RV64 target lowering. Current representative accesses require missing
  producer facts, not target inference.

## Watchouts

- Do not infer pointer-value memory ownership, layout, range, or provenance in
  RV64 from raw pointer values, integer casts, filenames, function names,
  object labels, or dump shape.
- Keep `layout_authority=unknown` and `range_verdict=unknown_compatible`
  fail-closed until producer facts prove stronger authority.
- Do not fold store-source/global-memory publication, direct-global
  return/select-chain, or terminator/select publication work into this plan.
- Do not reopen pointer-cast movement from idea 429 or selected global
  object-data support from idea 433.
- Existing RV64 pointer-value tests are manually-authored consumer fixtures;
  they are not proof that `930930-1` has producer authority.

## Proof

Step 1 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
