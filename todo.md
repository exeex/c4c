Status: Active
Source Idea Path: ideas/open/625_prepared_stack_slot_preservation_source_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh stack-slot preserve evidence

# Current Packet

## Just Finished

Completed Step 1 diagnostic refresh for ordinary same-module stack-slot
preserve rows. Ran the supervisor-delegated `bash -lc` proof command that
builds `c4cll`, captures prepared/codegen output for the selected rows, writes
artifacts under
`build/agent_state/625_step1_stack_slot_preserve_evidence/`, and appends the
diagnostic summary to `test_after.log`.

Rows captured:

- `tests/c/external/gcc_torture/src/20020529-1.c`: `prepared_rc=0`,
  `codegen_rc=2`; repeated `%p.b` stack-slot preserves use
  `preservation_source=register:value#1` and
  `preservation_destination=stack_slot:slot#19:value#1`.
- `tests/c/external/gcc_torture/src/20000412-4.c`: `prepared_rc=0`,
  `codegen_rc=2`; `%p.j` and `%p.width` stack-slot preserves use
  `preservation_source=register:value#1/#3` and destination slots
  `slot#7+stack16` / `slot#8+stack20`.
- `tests/c/external/gcc_torture/src/pr51933.c`: `prepared_rc=0`,
  `codegen_rc=2`; `%p.y` uses
  `preservation_source=register:value#4` into `slot#60+stack16`, while
  `@.str0` stack-slot preservation already carries
  `preservation_source=stack_slot:slot#1100:value#2199`.

First-owner buckets from this refresh:

- Prepared producer-authority candidate: caller-saved stack-slot reuse
  preserves publish value identity and destination stack-slot facts, but the
  ordinary register-source cases still expose source as `register:value#N`
  rather than a concrete source endpoint such as a register name or source
  location.
- RV64/prepared consumer residual: `20000412-4.c` currently stops at
  `rv64_prepared_move_bundle_consumer` for an ambiguous non-parallel
  register-source fan-in to one stack destination.
- Ordinary call ABI/result residual: `20020529-1.c` currently stops before
  object emission on `unsupported_call_abi` for ordinary same-module
  call/result lowering in `foo`.
- Non-goal guard residual: `pr51933.c` currently stops on
  `unsupported_inline_asm_fragment`, so inline-asm carrier policy remains out
  of scope for this source idea.

Negative guard rows:

- Keep `register:value#N` source-only preserve facts fail-closed until Step 2
  identifies the producer authority that can publish concrete source endpoint
  facts.
- Keep `stack_slot:slot#N:value#N` source facts separate from ordinary
  caller-saved register-source preserves; do not use them to infer register
  sources.
- Preserve the current non-goal blockers for ambiguous move bundles,
  ordinary call/result lowering, and inline asm instead of broadening this
  idea into those owners.

## Suggested Next

Execute Step 2 from `plan.md`: inspect the prepared call-boundary producer
surface that emits caller-saved stack-slot reuse preserves, then identify where
concrete source endpoint facts can be published before RV64 object emission.

## Watchouts

- Do not infer preserve source registers inside RV64 from ABI parameter
  position, function storage summaries, source filename, testcase shape, or
  final assembly layout.
- Keep `20020529-1.c` and adjacent ordinary-call preserve rows as breadth
  or guard evidence, not named-case implementation targets.
- Do not broaden into outgoing stack argument destination offsets,
  variadic/library policy, runtime mismatch, local/global producers,
  stack-frame consumers, expectations, unsupported markers, allowlists,
  timeouts, or accounting changes.
- The delegated summary loop also matched the newly-created `summary.txt`
  file, producing a final empty `## summary.txt` section and stderr warnings
  from `cat`/`rg`; the three row artifact directories and `test_after.log`
  summary were still produced by the exact command.

## Proof

Ran exactly the supervisor-delegated proof command from `/workspaces/c4c`.
`test_after.log` contains the `cmake --build build --target c4cll` proof plus
the appended diagnostic summary. Artifacts are in
`build/agent_state/625_step1_stack_slot_preserve_evidence/`.
