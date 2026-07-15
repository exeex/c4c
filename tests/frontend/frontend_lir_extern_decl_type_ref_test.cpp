#include "ir.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace {

[[noreturn]] void fail(const std::string& msg) {
  std::cerr << "FAIL: " << msg << "\n";
  std::exit(1);
}

void expect_true(bool cond, const std::string& msg) {
  if (!cond) fail(msg);
}

void expect_eq(std::string_view actual, std::string_view expected,
               const std::string& msg) {
  if (actual != expected) {
    fail(msg + "\nExpected: " + std::string(expected) +
         "\nActual: " + std::string(actual));
  }
}

c4c::codegen::lir::LirModule make_struct_module() {
  c4c::codegen::lir::LirModule module;
  module.link_name_texts = std::make_shared<c4c::TextTable>();
  module.link_names.attach_text_table(module.link_name_texts.get());
  module.struct_names.attach_text_table(module.link_name_texts.get());

  const c4c::StructNameId pair_id = module.struct_names.intern("%struct.Pair");
  c4c::codegen::lir::LirStructDecl pair_decl;
  pair_decl.name_id = pair_id;
  pair_decl.fields.push_back({c4c::codegen::lir::LirTypeRef("i32")});
  module.type_decls.push_back("%struct.Pair = type { i32 }");
  module.record_struct_decl(std::move(pair_decl));
  return module;
}

void push_recorded_extern_decl(c4c::codegen::lir::LirModule& module,
                               std::string_view name) {
  const auto it = module.extern_decl_name_map.find(std::string(name));
  expect_true(it != module.extern_decl_name_map.end(),
              "record_extern_decl should populate the legacy raw-name compatibility map");

  c4c::codegen::lir::LirExternDecl decl;
  decl.name = it->second.name;
  decl.return_type_str = it->second.return_type_str;
  decl.return_type = it->second.return_type;
  decl.link_name_id = it->second.link_name_id;
  module.extern_decls.push_back(std::move(decl));
}

void expect_link_name_id_preempts_legacy_raw_map() {
  c4c::codegen::lir::LirModule module = make_struct_module();
  const c4c::LinkNameId extern_pair_id =
      module.link_names.intern("extern_pair");

  module.record_extern_decl("extern_pair", "void", extern_pair_id);
  module.record_extern_decl("extern_pair", "%struct.Pair");

  expect_true(module.extern_decl_name_map.empty(),
              "known LinkNameId should preempt legacy raw-name fallback");
  const auto it = module.extern_decl_link_name_map.find(extern_pair_id);
  expect_true(it != module.extern_decl_link_name_map.end(),
              "known LinkNameId should remain the extern declaration authority");
  expect_eq(it->second.return_type_str, "%struct.Pair",
            "LinkNameId extern declaration should merge later raw-name return metadata");
  expect_true(it->second.return_type.has_struct_name_id(),
              "LinkNameId extern declaration should retain structured return metadata");
}

void expect_type_ref_structured_equality_uses_name_id(
    c4c::codegen::lir::LirModule& module) {
  const c4c::codegen::lir::LirTypeRef builtin_i32(
      c4c::codegen::lir::LirBuiltinType::I32);
  expect_true(builtin_i32.builtin_type() == c4c::codegen::lir::LirBuiltinType::I32 &&
                  builtin_i32.integer_bit_width() == 32,
              "builtin integer refs should retain their existing semantic fields");

  const c4c::StructNameId pair_id = module.struct_names.find("%struct.Pair");
  const c4c::StructNameId slot_id = module.struct_names.intern("%struct.Slot");
  expect_true(pair_id != c4c::kInvalidStructName,
              "fixture should declare Pair for equality collision checks");
  expect_true(slot_id != c4c::kInvalidStructName && slot_id != pair_id,
              "fixture should carry a distinct Slot id for equality collision checks");

  const c4c::codegen::lir::LirTypeRef pair_ref =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.Pair", pair_id);
  const c4c::codegen::lir::LirTypeRef collision_ref =
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.Pair", slot_id);
  expect_true(pair_ref != collision_ref,
              "extern type-ref equality should reject same text with different StructNameId");

  expect_true(c4c::codegen::lir::LirTypeRef("%struct.Pair") ==
                  c4c::codegen::lir::LirTypeRef("%struct.Pair"),
              "extern legacy no-id type refs should still compare by rendered text");

  const c4c::codegen::lir::LirTypeRef pair_union =
      c4c::codegen::lir::LirTypeRef::union_type("%struct.Pair", pair_id);
  expect_true(pair_ref.is_named_struct() && !pair_ref.is_named_union(),
              "named struct refs should retain their nominal composite kind");
  expect_true(pair_union.is_named_union() && !pair_union.is_named_struct(),
              "named union refs should retain their nominal composite kind");
  expect_true(pair_ref != pair_union,
              "named struct and union refs with one name id must remain distinct");

  const c4c::codegen::lir::LirTypeRef bytes =
      c4c::codegen::lir::LirTypeRef::array(
          c4c::codegen::lir::LirTypeRef::integer(8), 4);
  c4c::codegen::lir::LirTypeRef other_bytes =
      c4c::codegen::lir::LirTypeRef::array(
          c4c::codegen::lir::LirTypeRef::integer(8), 4);
  other_bytes.str() = "stale caller text";
  expect_true(bytes.kind() == c4c::codegen::lir::LirTypeKind::Array &&
                  bytes.has_array_shape(),
              "array factory should publish structured array semantics");
  expect_true(bytes.array_length() == 4 && bytes.array_element_type() != nullptr &&
                  bytes.array_element_type()->integer_bit_width() == 8,
              "array factory should retain element and length without text parsing");
  expect_eq(bytes.str(), "[4 x i8]",
            "array compatibility text should be derived from structural facts");
  expect_true(bytes == other_bytes,
              "structured array equality should use element and length, not stale text mirrors");

  const c4c::codegen::lir::LirTypeRef runtime_array =
      c4c::codegen::lir::LirTypeRef::runtime_text("[4 x i8]");
  expect_true(!runtime_array.has_array_shape(),
              "runtime text should remain an explicit unstructured compatibility boundary");
}

}  // namespace

int main() {
  expect_link_name_id_preempts_legacy_raw_map();

  c4c::codegen::lir::LirModule module = make_struct_module();
  expect_type_ref_structured_equality_uses_name_id(module);

  module.record_extern_decl("extern_pair", "%struct.Pair");
  push_recorded_extern_decl(module, "extern_pair");

  const auto& decl = module.extern_decls.front();
  expect_eq(decl.return_type_str, "%struct.Pair",
            "extern declaration should preserve return_type_str text");
  expect_eq(decl.return_type.str(), "%struct.Pair",
            "extern declaration return mirror text should match");
  expect_true(decl.return_type.has_struct_name_id(),
              "extern declaration return mirror should carry StructNameId");
  expect_eq(module.struct_names.spelling(decl.return_type.struct_name_id()),
            "%struct.Pair",
            "extern declaration StructNameId should resolve to return text");

  expect_eq(std::to_string(module.type_decls.size()), "1",
            "struct fixture should retain one legacy type_decls shadow");
  expect_eq(module.type_decls.front(), "%struct.Pair = type { i32 }",
            "legacy type_decls shadow should stay populated");

  c4c::codegen::lir::verify_module(module);
  const std::string llvm_ir = c4c::codegen::lir::print_llvm(module);
  expect_true(llvm_ir.find("%struct.Pair = type { i32 }") != std::string::npos,
              "printer should emit the structured struct_decls declaration");
  expect_true(llvm_ir.find("declare %struct.Pair @extern_pair(...)") !=
                  std::string::npos,
              "printer should keep using return_type_str for extern declarations");

  c4c::codegen::lir::LirModule structured_identity = module;
  structured_identity.extern_decls.front().return_type.str() =
      "%struct.StaleMirrorText";
  c4c::codegen::lir::verify_module(structured_identity);

  c4c::codegen::lir::LirModule mismatched_shadow = module;
  mismatched_shadow.type_decls.front() = "%struct.Pair = type { i64 }";
  try {
    c4c::codegen::lir::verify_module(mismatched_shadow);
    fail("verifier should reject mismatched struct_decls/type_decls shadows");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule missing_mirror = module;
  missing_mirror.extern_decls.front().return_type =
      c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(missing_mirror);
    fail("verifier should reject a known struct extern return without StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule text_fallback = module;
  text_fallback.extern_decls.front().return_type_str = "%struct.NotDeclared";
  try {
    c4c::codegen::lir::verify_module(text_fallback);
    fail("verifier should reject an extern return text mismatch without declared struct boundary");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  std::cout << "PASS: frontend_lir_extern_decl_type_ref\n";
  return 0;
}
