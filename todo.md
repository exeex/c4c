Status: Active
Source Idea Path: ideas/open/687_structured_layout_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Isolate Type Declaration And Typed Operand Compatibility

# Current Packet

## Just Finished

Completed `plan.md` Step 2 selected contraction for typed-index operand
lowering.

`ParsedTypedOperand` now records the adapter-private lowered scalar type when
raw LIR typed operand text is parsed, and `lower_typed_index_value` consumes
that cached type instead of re-entering integer type lowering from
`type_text`. The original compatibility type text remains available for the
existing adapter boundary, and produced index values are intended unchanged.

## Suggested Next

Choose the next structured-layout bridge contraction from the remaining
rendered-text lookup sites, likely an adapter-private type declaration or
aggregate layout lookup path that still re-enters raw text after a typed helper
has already resolved the relevant fact.

## Watchouts

This removes only the repeated scalar integer type parse in the typed-index
value consumer; typed operand text is still retained at the raw LIR adapter
boundary for compatibility and diagnostics. Avoid widening this into memory
addressing policy, public BIR type/model authority, prepared/prealloc, target
transport, MIR, initializer lowering, memory/provenance policy, or call ABI
placement.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed; 100% tests passed, 0 tests failed out of 302. Proof log:
`test_after.log`.
