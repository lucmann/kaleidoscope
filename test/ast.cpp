#include "ast.hpp"
#include "llvm_ir_visitor.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

template <typename Of, typename V> inline bool instanceof (V * v) {
  if (Of *b = dynamic_cast<Of *>(v)) {
    return true;
  }
  return false;
}

TEST(ast, should_able_to_parse_variable_ast) {
  tokenizer to;
  parser parser;
  std::list<std::unique_ptr<token>> toks = to.tokenize("id");
  translation_unit tu = parser.parse(toks);
  ASSERT_THAT(tu.size(), testing::Eq(1));
  testing::Matcher<ast::node *> matcher = testing::ResultOf(
      & instanceof <ast::variable, ast::node>, testing::IsTrue());
  EXPECT_TRUE(matcher.Matches(tu.front().get()));

  auto visitor = std::make_shared<llvm_ir_visitor>();
  tu.front()->accept(visitor);
  ASSERT_THAT(visitor->collect().size(), testing::Eq(1));
  llvm::Value **v = visitor->collect().front().get();
  ASSERT_TRUE(llvm::isa<llvm::Value>(**v));
}

TEST(ast, should_able_to_parse_number_ast) {
  tokenizer to;
  parser parser;
  std::list<std::unique_ptr<token>> toks = to.tokenize("1");
  translation_unit tu = parser.parse(toks);
  ASSERT_THAT(tu.size(), testing::Eq(1));
  testing::Matcher<ast::node *> matcher = testing::ResultOf(
      & instanceof <ast::number, ast::node>, testing::IsTrue());
  EXPECT_TRUE(matcher.Matches(tu.front().get()));

  auto visitor = std::make_shared<llvm_ir_visitor>();
  tu.front()->accept(visitor);
  ASSERT_THAT(visitor->collect().size(), testing::Eq(1));
  llvm::Value **v = visitor->collect().front().get();
  ASSERT_TRUE(llvm::isa<llvm::ConstantFP>(**v));
}

TEST(ast, should_able_to_parse_def_ast) {
  tokenizer to;
  parser parser;
  std::list<std::unique_ptr<token>> toks = to.tokenize("def test(a) {"
                                                       "a;"
                                                       "}");
  translation_unit tu = parser.parse(toks);
  ASSERT_THAT(tu.size(), testing::Eq(1));
  testing::Matcher<ast::node *> matcher = testing::ResultOf(
      & instanceof <ast::function_definition, ast::node>, testing::IsTrue());
  EXPECT_TRUE(matcher.Matches(tu.front().get()));
}

TEST(ast, should_able_to_parse_multiple_argument_def) {
  tokenizer to;
  parser parser;
  std::list<std::unique_ptr<token>> toks = to.tokenize("def test(a,b) {"
                                                       "a+b;"
                                                       "}");
  translation_unit tu = parser.parse(toks);
  ASSERT_THAT(tu.size(), testing::Eq(1));
  testing::Matcher<ast::node *> matcher = testing::ResultOf(
      & instanceof <ast::function_definition, ast::node>, testing::IsTrue());
  EXPECT_TRUE(matcher.Matches(tu.front().get()));
  auto visitor = std::make_shared<llvm_ir_visitor>();
  tu.front()->accept(visitor);
  ASSERT_THAT(visitor->collect().size(), testing::Eq(1));
  llvm::Value **v = visitor->collect().front().get();
  ASSERT_TRUE(llvm::isa<llvm::Function>(**v));

  auto f = llvm::dyn_cast<llvm::Function>(*v);
  ASSERT_THAT(f->arg_size(), testing::Eq(2));
}

TEST(ast, should_able_to_parse_the_func_call) {
  tokenizer to;
  parser parser;
  std::list<std::unique_ptr<token>> toks = to.tokenize("def test(a){}"
                                                       "test(1)");
  translation_unit tu = parser.parse(toks);
  ASSERT_THAT(tu.size(), testing::Eq(2));
  auto visitor = std::make_shared<llvm_ir_visitor>();
  tu.front()->accept(visitor);
  tu.pop_front();
  tu.front()->accept(visitor);
  auto inst = visitor->collect();
  ASSERT_THAT(inst.size(), testing::Eq(3));
  inst.pop_front();
  llvm::Value **v = inst.front().get();
  ASSERT_TRUE(llvm::isa<llvm::CallInst>(**v));

  auto call = llvm::dyn_cast<llvm::CallInst>(*v);
  ASSERT_THAT(call->arg_size(), testing::Eq(1));
  auto argsIterator = call->args().begin();
  ASSERT_TRUE(llvm::isa<llvm::ConstantFP>(*argsIterator));
  llvm::APFloat arg =
      llvm::dyn_cast<llvm::ConstantFP>(*argsIterator)->getValueAPF();
  ASSERT_THAT(arg.convertToFloat(), testing::FloatEq(1.0f));
}

TEST(ast, should_able_to_parse_the_double_parameter_func_call) {
  tokenizer to;
  parser parser;
  std::list<std::unique_ptr<token>> toks = to.tokenize("def test(a, b){}"
                                                       "test(1, 3)");
  translation_unit tu = parser.parse(toks);
  ASSERT_THAT(tu.size(), testing::Eq(2));
  auto visitor = std::make_shared<llvm_ir_visitor>();
  tu.front()->accept(visitor);
  tu.pop_front();
  tu.front()->accept(visitor);
  auto inst = visitor->collect();
  ASSERT_THAT(inst.size(), testing::Eq(4));
  inst.pop_front();
  llvm::Value **v = inst.front().get();
  ASSERT_TRUE(llvm::isa<llvm::CallInst>(**v));

  auto call = llvm::dyn_cast<llvm::CallInst>(*v);
  ASSERT_THAT(call->arg_size(), testing::Eq(2));
  auto argsIterator = call->args().begin();
  ASSERT_TRUE(llvm::isa<llvm::ConstantFP>(*argsIterator));
//
//  ASSERT_THAT(llvm::dyn_cast<llvm::ConstantFP>(*argsIterator)
//                  ->getValueAPF()
//                  .convertToFloat(),
//              testing::FloatEq(3.0f));
//  ASSERT_THAT(llvm::dyn_cast<llvm::ConstantFP>(*(++argsIterator))
//                  ->getValueAPF()
//                  .convertToFloat(),
//              testing::FloatEq(3.0f));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
