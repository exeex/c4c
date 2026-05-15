Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Add Runtime Helper Boundary Records

# Current Packet

## Just Finished

Step 6 inspected the prepared call-boundary and AArch64 selected-record
surfaces for i128 runtime helper lowering. The packet is blocked before target
selection because the required prepared/shared helper authority does not exist.

Blocker:

- `PreparedCallPlan` represents retained `bir::CallInst` call boundaries, but
  i128 helper-needed operations such as div/rem and float/i128 conversions are
  still retained as scalar `bir::BinaryInst`/`bir::CastInst` operation shapes.
- There is no prepared i128 helper-call carrier that maps a source i128
  operation to a required helper family or callee symbol.
- There is no prepared helper boundary record carrying low/high argument lane
  bindings, low/high result lane bindings or memory-return ownership, and
  helper-specific clobber/resource policy for those source operations.
- Existing generic `PreparedCallArgumentPlan`, `PreparedCallResultPlan`, and
  `PreparedClobberedRegister` facts are useful once a real helper call exists,
  but they are not tied to i128 operation identity and cannot be consumed
  without inventing target-local helper selection or fixed-register marshaling.

No AArch64 helper records were added in this packet.

## Suggested Next

Hand this back as a prepared/shared authority packet before continuing Step 6:
define explicit prepared i128 runtime-helper facts for helper-required
families, including helper kind/callee identity, source operation identity,
low/high argument and result lane ownership, memory-return policy where
required, clobbers/resources, and ABI/register-bank transition facts. After
that exists, resume Step 6 selected-record consumption.

## Watchouts

- Do not create a local i128 allocator in AArch64 codegen.
- Do not infer helper ABI from rendered names, numeric register adjacency, or
  fixed `x0`/`x1` accumulator conventions.
- Do not lower i128 helper-required operations as scalar i64 shortcuts.
- Do not synthesize helper calls directly in AArch64 dispatch from
  `BinaryInst`/`CastInst` opcodes; the helper family and callee identity must
  come from prepared/shared facts.
- Generic retained-call `PreparedCallPlan` facts are not enough by themselves
  for i128 helper lowering because they do not preserve source operation
  identity or low/high i128 lane authority.
- Printer output remains deferred. This packet did not touch terminal printer
  paths.

## Proof

Proof command run:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_aarch64_target_instruction_records|backend_aarch64_target_record_core_contract|backend_aarch64_mir_carrier|backend_aarch64_instruction_dispatch)$') > test_after.log 2>&1`

Result: passed, 5/5 selected tests green. Proof log: `test_after.log`.

Additional hygiene: `git diff --check` passed.
