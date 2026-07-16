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
                  builtin_i32.kind() == c4c::codegen::lir::LirTypeKind::Integer &&
                  builtin_i32.integer_bit_width() == 32,
              "builtin integer refs should retain their existing semantic fields");
  const c4c::codegen::lir::LirTypeRef integer_i32 =
      c4c::codegen::lir::LirTypeRef::integer(32);
  const c4c::codegen::lir::LirTypeRef integer_i64 =
      c4c::codegen::lir::LirTypeRef::integer(64);
  expect_true(integer_i32 == builtin_i32 && integer_i32 != integer_i64,
              "integer factories should preserve width-sensitive equality");

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

  expect_eq(other_bytes.render_llvm(), "[4 x i8]",
            "structured array rendering should use element and length");

  c4c::codegen::lir::LirStructDecl storage_decl;
  storage_decl.name_id = module.struct_names.intern("%struct.Storage");
  storage_decl.fields.push_back({other_bytes});
  expect_eq(c4c::codegen::lir::render_struct_decl_llvm(module, storage_decl),
            "%struct.Storage = type { [4 x i8] }",
            "structured declaration rendering should use structural array facts");

  const c4c::codegen::lir::LirTypeRef runtime_array("[4 x i8]");
  expect_true(!runtime_array.has_array_shape(),
              "runtime text should remain an explicit unstructured compatibility boundary");
}

void expect_extern_byval_parameter_printer_uses_signature_store() {
  c4c::codegen::lir::LirModule module = make_struct_module();
  const c4c::LinkNameId extern_big_id =
      module.link_names.intern("extern_big");
  const c4c::StructNameId big_id =
      module.struct_names.intern("%struct.Big");
  c4c::codegen::lir::LirStructDecl big_decl;
  big_decl.name_id = big_id;
  big_decl.fields.push_back({c4c::codegen::lir::LirTypeRef::integer(64)});
  module.type_decls.push_back("%struct.Big = type { i64 }");
  module.record_struct_decl(std::move(big_decl));

  module.record_extern_decl("extern_big", "i32", extern_big_id);
  c4c::codegen::lir::LirCallSignature signature;
  signature.return_type_ref = c4c::codegen::lir::LirTypeRef::integer(32);
  signature.fixed_param_types = {"ptr byval(%struct.StaleTextOnly) align 1"};
  signature.fixed_param_type_refs = {
      c4c::codegen::lir::LirTypeRef::struct_type("%struct.Big", big_id)};
  const c4c::codegen::lir::LirFunctionSignatureRef signature_ref =
      module.register_extern_function_signature("extern_big", extern_big_id,
                                                signature);

  const auto it = module.extern_decl_link_name_map.find(extern_big_id);
  expect_true(it != module.extern_decl_link_name_map.end(),
              "link-backed extern declaration should remain map-owned");
  c4c::codegen::lir::LirExternDecl decl;
  decl.name = "stale_raw_extern_big";
  decl.return_type_str = "i32";
  decl.return_type = c4c::codegen::lir::LirTypeRef::integer(32);
  decl.link_name_id = extern_big_id;
  decl.function_signature_ref = signature_ref;
  module.extern_decls.push_back(std::move(decl));

  c4c::codegen::lir::verify_module(module);
  const std::string llvm_ir = c4c::codegen::lir::print_llvm(module);
  expect_true(llvm_ir.find("declare i32 @extern_big(ptr byval(%struct.Big))") !=
                  std::string::npos,
              "extern byval aggregate declaration should render parameters from the signature store");
  expect_true(llvm_ir.find("%struct.StaleTextOnly") == std::string::npos &&
                  llvm_ir.find("stale_raw_extern_big") == std::string::npos,
              "extern byval aggregate declaration should not recover authority from stale text");
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
  expect_extern_byval_parameter_printer_uses_signature_store();

  c4c::codegen::lir::LirModule structured_identity = module;
  const c4c::StructNameId extern_pair_id =
      structured_identity.extern_decls.front().return_type.struct_name_id();
  structured_identity.extern_decls.front().return_type =
      c4c::codegen::lir::LirTypeRef::struct_type(
          "%struct.StaleMirrorText", extern_pair_id);
  c4c::codegen::lir::verify_module(structured_identity);

  c4c::codegen::lir::LirModule printer_identity = module;
  printer_identity.extern_decls.front().return_type_str =
      "%struct.StaleReturnText";
  printer_identity.extern_decls.front().return_type =
      c4c::codegen::lir::LirTypeRef::struct_type(
          "%struct.StaleMirrorText", extern_pair_id);
  const std::string identity_ir =
      c4c::codegen::lir::print_llvm(printer_identity);
  expect_true(identity_ir.find("declare %struct.Pair @extern_pair(...)") !=
                  std::string::npos,
              "printer should render extern aggregate returns from StructNameId authority");
  expect_true(identity_ir.find("%struct.StaleReturnText") == std::string::npos &&
                  identity_ir.find("%struct.StaleMirrorText") == std::string::npos,
              "printer should not recover extern return authority from stale text");

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

  c4c::codegen::lir::LirModule wrong_struct_id = module;
  wrong_struct_id.extern_decls.front().return_type =
      c4c::codegen::lir::LirTypeRef::struct_type(
          "%struct.Pair", module.struct_names.intern("%struct.Slot"));
  try {
    c4c::codegen::lir::verify_module(wrong_struct_id);
    fail("verifier should reject an extern return with the wrong StructNameId");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  c4c::codegen::lir::LirModule text_fallback = module;
  text_fallback.extern_decls.front().return_type_str = "%struct.NotDeclared";
  text_fallback.extern_decls.front().return_type =
      c4c::codegen::lir::LirTypeRef("%struct.Pair");
  try {
    c4c::codegen::lir::verify_module(text_fallback);
    fail("verifier should reject an extern return text mismatch without declared struct boundary");
  } catch (const c4c::codegen::lir::LirVerifyError&) {
  }

  std::cout << "PASS: frontend_lir_extern_decl_type_ref\n";
  return 0;
}
