Status: Active
Source Idea Path: ideas/open/440_direct_global_return_select_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Direct-Global Evidence

# Current Packet

## Just Finished

Completed Step 1: audited direct-global return/select-chain evidence for idea
440 and recorded classifications under
`build/agent_state/440_step1_direct_global_audit/`.

Direct-global bucket table:

| bucket | function/block | raw BIR shape | prepared fact | global symbol | value home or terminator use | current authority state | first missing fact |
| --- | --- | --- | --- | --- | --- | --- | --- |
| direct-global return candidate | `foo/block_1` | `bir.ret ptr @global` | `home @global value_id=4 kind=register reg=t0`; `move_bundle phase=before_return authority=none block_index=1 instruction_index=3` | `@global` | return terminator uses direct global pointer; ABI return move exists | fail-closed candidate | explicit direct-global return authority tying terminator use, global symbol identity, value home/storage, and ABI return use |
| direct-global select-chain candidate | `foo/block_1` | `%t3 = bir.load_global i32 @global`; `%t4 = bir.add i32 %t3, 1`; `bir.store_global @global, i32 %t4` | `select_chain value=%t3 direct_global_select_chain=yes root_inst=0 source_producer=load_global`; `value=%t4 ... source_producer=binary` | `@global` | `%t3`/`%t4` have register homes; store-source also marks direct-global chain | candidate fact, not authority | accepted direct-global select-chain authority contract/predicate and consumer boundary |
| direct-global call-argument select-chain | `bar/entry` | `%t0 = bir.load_global i32 @global`; `bir.call ptr foo(i32 %t0)` | `call_arg_source index=0 direct_global_select_chain=yes direct_global_source=%t0 direct_global_root_inst=0`; prepared call arg carries same flag | `@global` | `%t0` has register home and call-arg ABI move to `a0` | candidate fact, not authority | contract deciding whether call arguments may consume direct-global select-chain facts |
| direct-global select-chain candidate | `bar/block_5` | `%t9 = bir.load_global i32 @global`; `%t10 = bir.add i32 %t9, 1`; `bir.store_global @global, i32 %t10` | `select_chain value=%t9 direct_global_select_chain=yes root_inst=0 source_producer=load_global`; `value=%t10 ... source_producer=binary` | `@global` | `%t9`/`%t10` have register homes; store-source marks direct-global chain | candidate fact, not authority | accepted direct-global select-chain authority contract/predicate and consumer boundary |
| completed adjacent store publication | `main/entry` | `bir.store_global @global, i32 1` | `source_producer=immediate`; destination `layout_authority=scalar_layout` | `@global` | store-global publication | complete elsewhere | none for idea 440; completed by ideas 446/447 |
| out-of-scope local publication | `bar/entry` | `bir.store_local %lv.p, ptr %t1` | `intent=store_local_publication source_producer=unknown` | n/a | frame-slot store | local publication gap | local publication owner, not direct-global authority |
| target terminator residual | module object route | object probe reports `unsupported_terminator_fragment` | object diagnostic only | n/a | BIR terminator lowering | target consumer gap | general terminator/select work belongs to idea 441 unless narrowed to explicit direct-global return authority |

Fresh probes confirm the latest store/global prerequisites are current:
`20041112-1 main.entry.0` now has `source_producer=immediate`, and global
memory rows have `layout_authority=scalar_layout`. The direct-global return row
still has only raw return plus prepared home/storage/move evidence, not an
accepted direct-global return authority fact.

## Suggested Next

Execute Step 2: define the direct-global authority contract. The first bounded
implementation packet after the contract should start with producer/prepared
contract coverage for direct-global return authority: raw `bir.ret ptr
@global` is accepted only with explicit prepared authority tying the return
terminator, global symbol identity, value home/storage, and ABI return use.
Select-chain and call-argument facts should be included in the Step 2 contract
but not broadened into generic terminator/select publication.

## Watchouts

- Keep this plan limited to direct-global return/select-chain authority.
- Do not fold general terminator/select publication into this plan; that
  belongs to `ideas/open/441_terminator_select_publication_authority.md`.
- Do not infer return authority or select-chain roots from raw
  `bir.ret ptr @global`, symbol spelling, select shape, testcase names, or one
  dump layout.
- Keep missing or incoherent direct-global authority fail-closed.
- Treat existing `direct_global_select_chain=yes` rows as candidate facts until
  Step 2 defines the accepted authority predicate and consumer boundary.
- Keep completed store/global publication facts from ideas 446/447 out of the
  direct-global packet.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log` or `test_after.log` during activation.

## Proof

Step 1 classification-only validation:

```sh
git diff --check
```
