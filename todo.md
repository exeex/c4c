Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select I128 Transport Nodes

# Current Packet

## Just Finished

Step 3 selected initial AArch64 i128 transport records from complete
`PreparedI128Carrier` facts, without arithmetic, helper-call, or printer scope.

Completed work:

- Added `I128TransportRecord` selected machine-node records with explicit
  transport kind, value identity, low/high lane order, lane width, total
  size/alignment, register-pair lane operands, memory-backed lane offsets, and
  source carrier provenance.
- Added fail-closed prepared-carrier conversion for missing carriers,
  incomplete carriers, unsupported carrier kinds, register conversion failures,
  missing memory operands, and memory size mismatches.
- Wired prepared AArch64 dispatch for i128 frame-slot load/store transport to
  consume `PreparedI128Carrier` and `PreparedMemoryAccess` facts directly.
- Added focused record and dispatch tests proving complete register-pair
  carrier selection, frame-slot load transport selection, and missing-carrier
  diagnostics.

## Suggested Next

Execute Step 4 from `plan.md`: select i128 arithmetic and bitwise nodes from
complete prepared carrier facts. Keep shifts, comparisons, helper-call
lowering, and terminal printer output deferred until their planned packets.

## Watchouts

- Do not create a local i128 allocator in AArch64 codegen.
- Do not infer low/high pair homes from rendered register names, numeric
  adjacency, or fixed `x0`/`x1` accumulator conventions.
- Do not lower i128 as scalar i64 or claim progress through named testcase
  shortcuts.
- Consume `PreparedI128Carrier` lane fields directly; generic
  `contiguous_width == 2` or occupied-register order is not semantic lane
  authority by itself.
- Current transport selection covers prepared frame-slot load/store shapes
  where structured memory and carrier facts are complete. Other i128 operation
  families remain explicitly deferred to later plan steps.
- ABI/helper-call pair facts are not complete in this packet. If arithmetic or
  helper selection needs call-boundary pair policy, stop and report the missing
  prepared/shared policy instead of inventing it in target codegen.
- Keep binary128/F128 and float/i128 conversions separate until helper-call
  facts carry explicit register-bank transition authority.

## Proof

Proof command run:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_aarch64_target_instruction_records|backend_aarch64_target_record_core_contract|backend_aarch64_mir_carrier|backend_aarch64_instruction_dispatch)$') > test_after.log 2>&1`

Result: passed, 5/5 selected tests green. Proof log: `test_after.log`.

Supervisor full-suite acceptance also passed: `(cmake --build build -j2 &&
ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`, with
regression guard accepted against full-suite `test_before.log`; 3167/3167
before and 3167/3167 after.

Additional hygiene: `git diff --check` passed.
