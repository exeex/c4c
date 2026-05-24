Status: Active
Source Idea Path: ideas/open/call-boundary-move-classification-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Prealloc Call-Boundary Classification API

# Current Packet

## Just Finished

Step 2 added the prealloc-owned target-neutral call-boundary move
classification API.

`classify_prepared_call_boundary_move(...)` now returns decoded Prepared facts
for a move without constructing MIR operands: phase, destination role, storage
kind, ABI index, source Prepared call plan/bundle/move pointers, matched call
argument/result plan, matched ABI binding, and authority status.

The helper covers available classifications plus unsupported op kind, missing
ABI index, missing call argument plan, missing call result plan, mismatched
call result plan, and missing ABI binding. It preserves higher-level Prepared
authority pointers when a later target mapper needs to choose diagnostics or
emission locally.

Focused direct coverage was added in
`backend_prealloc_call_boundary_classification` for register arguments,
immediate arguments without source value ids, structured stack-lane argument
matching with mismatched source value ids, call results, function-return moves,
ordinary value moves, and missing or mismatched authority statuses. The
stack-lane match is based on Prepared ABI index, frame-slot source encoding,
stack destination kind, and destination stack offset, not target/codegen reason
strings.

## Suggested Next

Step 3 should adapt AArch64 call-boundary operand/move resolution to consume
the prealloc classification helper while keeping AAPCS64 policy, register
conversion, diagnostics, MIR operand construction, printing, and instruction
emission target-local.

## Watchouts

The API deliberately reports facts and status only. Do not move AArch64
register spelling, AAPCS64 sret/byval/f128 policy, memory/immediate/symbol MIR
operand construction, printed records, or emitted instruction selection into
prealloc when adapting the consumer.

The helper currently classifies move-to-plan and move-to-binding authority. It
does not replace preservation lookup/emission yet; that should remain a
separate extraction if needed.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 153/153 backend tests. Proof log: `test_after.log`.
