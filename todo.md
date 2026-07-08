Status: Active
Source Idea Path: ideas/open/612_rv64_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Pointer BinaryInst Ownership

# Current Packet

## Just Finished

Completed Step 1, "Refresh Pointer BinaryInst Ownership", using current
`build/agent_state/612_step3_failure_reason_rows.tsv` diagnostics plus the
current RV64 gcc torture failed-list count (`967` rows). The refreshed
instruction-fragment population has `115` `unsupported_instruction_fragment`
rows; the in-scope pointer `BinaryInst` consumer family is still `23` rows.

Refreshed first-owner groups:
- RV64 pointer `BinaryInst` consumer, positive Step 2 family: `23` rows with
  `unsupported_instruction_fragment`, `instruction_kind=BinaryInst`, and
  `owner=ptr`: `src/20000801-1.c`, `src/20050502-1.c`,
  `src/20120808-1.c`, `src/930526-1.c`, `src/960327-1.c`,
  `src/961125-1.c`, `src/961213-1.c`, `src/990811-1.c`,
  `src/builtin-prefetch-4.c`, `src/loop-13.c`, `src/loop-2f.c`,
  `src/loop-2g.c`, `src/mode-dependent-address.c`, `src/pr20527-1.c`,
  `src/pr36038.c`, `src/pr41395-2.c`, `src/pr41463.c`,
  `src/pr43560.c`, `src/pr59643.c`, `src/pr62151.c`, `src/ssad-run.c`,
  `src/strct-pack-3.c`, `src/usad-run.c`.
- BIR pointer/address producer or architecture gap: `5`
  `unsupported_pointer_arithmetic` rows requiring prepared pointer arithmetic
  lowering for loaded pointer base plus scaled integer byte offset:
  `src/20021120-1.c`, `src/990524-1.c`, `src/loop-2c.c`,
  `src/pr27073.c`, `src/pr38212.c`.
- Selected pointer/local-memory authority, owned by idea 614: `35`
  `unsupported_local_memory_access` rows; representative rows include
  `src/20000722-1.c`, `src/20010123-1.c`, `src/20010605-2.c`,
  `src/pr30185.c`, `src/ptr-arith-1.c`.
- Branch stack-source owner: `10` `unsupported_branch_stack_load_authority` or
  `unsupported_branch_stack_load_source_freshness` rows; representatives:
  `src/20000314-3.c`, `src/20001017-1.c`, `src/930930-1.c`.
- Select publication owner: `7` pointer `SelectInst`
  `unsupported_instruction_fragment` rows plus `14` select-publication
  move-bundle rows; representatives: `src/20000815-1.c`,
  `src/930614-2.c`, `src/950426-1.c`, `src/20000706-1.c`,
  `src/pr45034.c`.
- ABI/call/frame/parameter owner: `69` rows across `unsupported_call_abi`,
  `unsupported_stack_frame`, and `unsupported_param_home`; representatives:
  `src/20000603-1.c`, `src/20020314-1.c`, `src/20020413-1.c`.
- Move-bundle owner: `37` `unsupported_move_bundle_target_shape` rows,
  including the `14` select-publication move-bundle rows; representatives:
  `src/20000422-1.c`, `src/20001026-1.c`, `src/pr71631.c`.
- Global owner: `24` `unsupported_global_data` rows; representatives:
  `src/20020213-1.c`, `src/20030828-1.c`, `src/pr36034-1.c`.
- Runtime owner: no rows in this diagnostics file matched runtime text; keep
  runtime mismatches out of this packet and under idea 618.
- Inline asm owner: `21` `unsupported_inline_asm_fragment` policy rows;
  representatives: `src/20001009-2.c`, `src/pr40022.c`,
  `src/pr51933.c`.
- Terminator owner: `25` `unsupported_terminator_fragment` rows;
  representatives: `src/20000801-2.c`, `src/20030910-1.c`,
  `src/pr56962.c`.
- Cast owner: `27` `CastInst` instruction-fragment rows; representatives:
  `src/20010604-1.c`, `src/20020506-1.c`, `src/pr43835.c`.
- Policy/publication tail: `7` `unsupported_scalar_compare_publication` rows;
  representatives: `src/20000731-1.c`, `src/20011217-1.c`,
  `src/loop-8.c`.

Negative guard rows for Step 2 include the scalar/non-pointer `BinaryInst`
rows (`10` i16 and `9` i8 rows). Keep `src/931110-1.c` as the scalar
`ashr` negative guard: refreshed evidence still shows it as a singleton
`BinaryInst`/`owner=i16` guard for this route, while `src/ashrdi-1.c` is a
move-bundle row and `src/pr28289.c` is a `CallInst` fragment, not broader
same-family scalar `ashr` breadth.

## Suggested Next

Step 2 should implement the first RV64 pointer `BinaryInst` consumer family for
the `23` positive `unsupported_instruction_fragment` rows listed above. Start
with several positive representatives such as `src/20000801-1.c`,
`src/930526-1.c`, `src/loop-2f.c`, `src/pr41395-2.c`, and
`src/strct-pack-3.c`, and prove that the consumer requires explicit upstream
pointer/address facts.

Use negative guards from the producer/authority rows
(`src/20021120-1.c`, `src/990524-1.c`, `src/20000722-1.c`,
`src/ptr-arith-1.c`), selected/branch/select/cast/ABI/global/move-bundle
owners (`src/930930-1.c`, `src/20000815-1.c`, `src/20010604-1.c`,
`src/20000603-1.c`, `src/20020213-1.c`, `src/20000422-1.c`), inline asm and
terminator policy rows (`src/pr40022.c`, `src/20000801-2.c`), and the scalar
`ashr` guard `src/931110-1.c`.

## Watchouts

- The positive pointer rows are classified by first-stop diagnostics only; the
  current case directories for sampled rows retain `case.log` but not prepared
  dumps, so Step 2 should use focused object probes before editing code if it
  needs operand-level details.
- Do not fold the `5` explicit `unsupported_pointer_arithmetic` rows into the
  consumer family; they name a prepared pointer arithmetic production or
  architecture gap.
- Do not mix pointer `BinaryInst` / address-authority work with idea 614
  pointer local-memory consumption, cast rows, ABI/call rows, select
  publication, branch freshness, move-bundle, global/runtime, inline asm,
  terminator, scalar compare publication, or policy-owned residuals.
- Keep `src/931110-1.c` / scalar `ashr` as a negative guard unless future
  refreshed evidence finds broader same-family scalar `ashr` breadth.
- Do not weaken expectations, unsupported markers, allowlists, timeout
  behavior, runtime handling, or GCC torture classification metadata.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. Proof log: `test_after.log`.
