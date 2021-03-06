#include "llvm-c/Core.h"

#include "codegen/LLVMFunctionCompiler.hh"
#include "core/expr.hh"
#include "core/prettyprinter.hh"
#include "utils/ansicolor.hh"
#include <algorithm>

LLVMTypeRef coral::codegen::LLVMFunctionCompiler::LLVMTypeFromCoral(coral::type::Type * t) {
  if (!t) return LLVMVoidTypeInContext(context);
  if (t->name == "Void") return LLVMVoidTypeInContext(context);
  if (t->name == "Ptr") return LLVMPointerType(LLVMInt8TypeInContext(context), 0);
  if (t->name == "Bool") return LLVMInt1TypeInContext(context);
  if (t->name == "Int8") return LLVMInt8TypeInContext(context);
  if (t->name == "Int16") return LLVMInt16TypeInContext(context);
  if (t->name == "Int32") return LLVMInt32TypeInContext(context);
  if (t->name == "Int64") return LLVMInt64TypeInContext(context);
  if (t->name == "Func") {
    // std::cout << "LLVMTyping: " << *t << "\n";
    auto ret_type = LLVMTypeFromCoral(&t->params.back());
    auto nparam = t->params.size() - 1;
    auto params = new LLVMTypeRef[nparam];
    for(ulong i=0; i<nparam; i++)
      params[i] = LLVMTypeFromCoral(&t->params[i]);
    auto ftype = LLVMFunctionType(
      ret_type,
      params, nparam,
      false);
    delete [] params;
    return ftype;
  }
  if (t->name == "Struct") {
    auto params = new LLVMTypeRef[t->params.size()];
    for(size_t i=0; i<t->params.size(); i++) {
      params[i] = LLVMTypeFromCoral(&t->params[i]);
    }
    return LLVMStructTypeInContext(context, params, t->params.size(), true);
  }
  if (t->name == "String") {
    return LLVMPointerType(LLVMInt8TypeInContext(context), 0);
  }
  std::cerr << COL_LIGHT_BLUE << "Warning: Unhandled Type: '" << *t << "'" << COL_CLEAR << "\n";
  return LLVMInt64TypeInContext(context);
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::Func * expr) {
  function = LLVMAddFunction(
    module,
    expr->name.c_str(),
    LLVMTypeFromCoral(expr->type.get()));
  (*info)[expr] = function;

  for(ulong i=0; i<expr->params.size(); i++) {
    (*info)[expr->params[i].get()] = LLVMGetParam(function, i);
  }
  if (expr->body) {
    basic_block = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilderAtEnd(builder, basic_block);
    expr->body->accept(this);

  }
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::IfExpr * expr) {
  auto thenblock = LLVMAppendBasicBlock(function, "then");
  auto elseblock = LLVMAppendBasicBlock(function, "else");
  auto endblock = LLVMAppendBasicBlock(function, "end");
  out = LLVMBuildCondBr(
    builder,
    compile(expr->cond.get()),
    thenblock,
    elseblock);
  int branchreturns = 0;
  returns = 0;
  LLVMPositionBuilderAtEnd(builder, thenblock);
  expr->ifbody->accept(this);
  if (!returns) LLVMBuildBr(builder, endblock);
  branchreturns += returns > 0 ? 1 : 0;

  returns = 0;
  LLVMPositionBuilderAtEnd(builder, elseblock);
  if (expr->elsebody) expr->elsebody->accept(this);
  if (!returns) LLVMBuildBr(builder, endblock);
  branchreturns += returns > 0 ? 1 : 0;

  // if we're returning from both branches, we didn't generate any
  // jumps to the ending basic block
  if (branchreturns == 2) {
    LLVMRemoveBasicBlockFromParent(endblock);
  } else {
    LLVMPositionBuilderAtEnd(builder, endblock);
  }
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::IntLiteral * expr) {
  out = LLVMConstInt(LLVMInt32TypeInContext(context), std::stol(expr->value), false);
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::StringLiteral * expr) {
  auto stringval = expr->getString();
  auto global = LLVMAddGlobal(
    module,
    LLVMArrayType(LLVMInt8TypeInContext(context), 1 + stringval.size()), "");
  auto llval = LLVMConstStringInContext(context, stringval.c_str(), stringval.size(), false);
  LLVMSetInitializer(global, llval);
  out = LLVMBuildBitCast(builder, global, LLVMPointerType(LLVMInt8TypeInContext(context), 0), "");
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::Var * var) {
  out = 0;
  if (info->find(var->expr) == info->end()) {
    // std::cerr << "Not Found: " << var->name << "\n";
    return;
  }
  switch (ast::ExprTypeVisitor::of(var->expr)) {
  case ast::ExprTypeKind::DefKind:
    out = info->find(var->expr)->second;
    // std::cout << "codegening: var def " << ((ast::Def *)var->expr)->name << "\n";
    // std::cout << "saved: " << LLVMPrintValueToString(out) << "\n";
    return;
  case ast::ExprTypeKind::FuncKind:
    // std::cout << "codegening: var fun " << ((ast::Func *)var->expr)->name << "\n";
    out = info->find(var->expr)->second;
    return;
  case ast::ExprTypeKind::LetKind:
    if (this->rawPointer) {
      out = info->find(var->expr)->second;
    } else {
      // A Let-expr usually generates a local -> Alloca
      // std::cerr << "Loading " << var->name << " = "
      //           << LLVMPrintValueToString(info->find(var->expr)->second) << " | "
      //           << LLVMPrintTypeToString(LLVMTypeOf(info->find(var->expr)->second))
      //           << "\n";
      out = LLVMBuildLoad(builder, info->find(var->expr)->second, var->name.c_str()) ;
    }
    return;
  default:
    std::cerr << "unknown var kind : " << var->name << " :: " << ast::ExprNameVisitor::of(var->expr) << "\n";
    break;
  }
  out = LLVMConstInt(LLVMInt32TypeInContext(context), 0, false);
}
void coral::codegen::LLVMFunctionCompiler::visit(ast::BinOp * expr) {
  auto lhs = compile(expr->lhs.get());
  auto rhs = compile(expr->rhs.get());
  if (LLVMGetTypeKind(LLVMTypeOf(lhs))==LLVMPointerTypeKind) {
    if (expr->op == "+") {
      // LLVMValueRef indices[1] = { rhs };
      out = LLVMBuildGEP(builder, lhs, &rhs, 1, "");
    } else {
      std::cerr << "Unknown Operator " << expr->op << " for Pointers\n";
      out = 0;
    }
    return;
  }
  if (expr->op == "-")
    out = LLVMBuildSub(builder, lhs, rhs, "");
  else if (expr->op == "+")
    out = LLVMBuildAdd(builder, lhs, rhs, "");
  else if (expr->op == "%")
    out = LLVMBuildSRem(builder, lhs, rhs, "");
  else if (expr->op == "*")
    out = LLVMBuildMul(builder, lhs, rhs, "");
  else if (expr->op == "/")
    out = LLVMBuildSDiv(builder, lhs, rhs, "");
  else if (expr->op == "=")
    out = LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "");
  else if (expr->op == "!=")
    out = LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "");
  else if (expr->op == "<")
    out = LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "");
  else if (expr->op == "<=")
    out = LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "");
  else if (expr->op == ">")
    out = LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "");
  else if (expr->op == ">=")
    out = LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "");
}
void coral::codegen::LLVMFunctionCompiler::visit(ast::Return * expr) {
  returns++;
  if (expr->val)
    out = LLVMBuildRet(builder, compile(expr->val.get()));
  else
    out = LLVMBuildRetVoid(builder);
}

namespace coral {
  LLVMValueRef ParseStruct(coral::codegen::LLVMFunctionCompiler * cc, coral::ast::Call * expr) {
    auto size = expr->arguments.size();
    LLVMTypeRef * fieldTypes = new LLVMTypeRef[size];
    LLVMValueRef * fieldValues = new LLVMValueRef[size];
    std::string * fieldNames  = new std::string[size];
    int i=0;
    for(auto && arg : expr->arguments) {
      if (ast::ExprTypeVisitor::of(arg.get()) != ast::ExprTypeKind::BinOpKind) return 0;
      auto binop = (ast::BinOp *)arg.get();
      if (binop->op != "=") return 0;
      auto lhs = dynamic_cast<ast::Var *>(binop->lhs.get());
      if (lhs == 0) {
        std::cerr << "FAIL " << ast::ExprNameVisitor::of(binop->lhs.get()) << "\n";
        return 0;
      }
      fieldNames[i] = lhs->name;
      fieldValues[i] = cc->compile(binop->rhs.get());
      fieldTypes[i] = LLVMTypeOf(fieldValues[i]);
      i++;
    }
    auto type = LLVMStructTypeInContext(cc->context, fieldTypes, size, true);
    auto val = LLVMBuildAlloca(cc->builder, type, "");
    LLVMValueRef index[2] = {
      LLVMConstInt(LLVMInt32TypeInContext(cc->context), 0, false),
      LLVMConstInt(LLVMInt32TypeInContext(cc->context), 0, false) };
    for(size_t j=0; j<expr->arguments.size(); j++) {
      index[1] = LLVMConstInt(LLVMInt32TypeInContext(cc->context), j, false);
      auto ptr = LLVMBuildGEP(cc->builder, val, index, 2, fieldNames[j].c_str());
      LLVMBuildStore(cc->builder, fieldValues[j], ptr);
    }
    // std::cerr << COL_LIGHT_RED;
    // std::cerr << "Struct Expression: " << LLVMPrintValueToString(val);
    // std::cerr << COL_CLEAR << "\n";
    return val;
  }
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::Call * expr) {

  if (ast::ExprTypeVisitor::of(expr->callee.get()) == ast::ExprTypeKind::VarKind) {
    expr->callee->accept(this);
    auto var = (ast::Var *)expr->callee.get();
    // TODO: make this an actual operator
    if (var->name == "addrof") {
      this->rawPointer = 1;
      expr->arguments[0]->accept(this);
      this->rawPointer = 0;
      return;
    } else if (var->name == "derefi") {
      // TODO typed pointers should eliminate the need for a "deref-as-integer" builtin
      expr->arguments[0]->accept(this);
      auto intval = out;
      auto ptrval = LLVMBuildIntToPtr(
        builder, intval,
        LLVMPointerType(LLVMInt32TypeInContext(context), 0), "ptrcast");
      out = LLVMBuildLoad(builder, ptrval, var->name.c_str());
      return;
    } else if (var->name == "struct") {
      out = ParseStruct(this, expr);
      out = LLVMBuildLoad(builder, out, "");
      return;
    }
    out = LLVMGetNamedFunction(module, var->name.c_str());
    if (!out)
      out = LLVMAddFunction(
        module, var->name.c_str(),
        LLVMFunctionType(LLVMVoidTypeInContext(context), 0, 0, true));
  } else if (!out) {
    std::cerr << "missing var " << ast::ExprNameVisitor::of(expr->callee.get()) << "\n";
  }
  auto llvmVarRef = out;
  auto llvmArgs = new LLVMValueRef[expr->arguments.size()];
  // std::cerr << "Function " << expr->callee.get() << "\n";
  // std::cerr << LLVMPrintValueToString(llvmVarRef) << "\n";
  for(ulong i=0; i<expr->arguments.size(); i++) {
    // std::cerr << "  arg[" << i << "]"<< expr->arguments[i].get() << "\n";
    expr->arguments[i]->accept(this);
    llvmArgs[i] = out;
  }
  out = LLVMBuildCall(
    builder, llvmVarRef,
    llvmArgs, expr->arguments.size(), "");
  delete [] llvmArgs;
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::Block * expr) {
  for(auto && line : expr->lines) if (line) line->accept(this);
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::Let * expr) {
  out = 0;
  expr->value->accept(this);
  auto llval = out;
  // TODO: LLVMTypeFromCoral(expr->type)
  auto local = LLVMBuildAlloca(builder, LLVMTypeOf(llval), expr->var->name.c_str());
  // std::cerr << COL_LIGHT_BLUE;
  // PrettyPrinter::print(expr->var.get());
  // std::cerr << " -- " << LLVMPrintTypeToString(LLVMTypeOf(llval))
  //           << COL_CLEAR << "\n";
  LLVMBuildStore(builder, llval, local);
  (*info)[expr] = local;
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::Set * expr) {
  out = 0;
  expr->value->accept(this);
  auto llval = out;
  auto local = (*info)[expr->var->expr];
  if (ast::ExprTypeVisitor::of(expr->var->expr) == ast::ExprTypeKind::DefKind) {
    std::cerr << "Warning: writing to a parameter is not currently supported\n";
    out = 0;
    return;
  }
  LLVMBuildStore(builder, llval, local);
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::Comment * w) { }

void coral::codegen::LLVMFunctionCompiler::visit(ast::Member * w) {
  this->rawPointer = 1;
  w->base->accept(this);
  this->rawPointer = 0;
  auto baseinstr = out;
  auto n = std::stoi(w->member.substr(5));
  LLVMValueRef index[2] = {
    LLVMConstInt(LLVMInt32TypeInContext(context), 0, false),
    LLVMConstInt(LLVMInt32TypeInContext(context), n, false)};
  out = LLVMBuildGEP(builder, baseinstr, index, 2, w->member.c_str());
  out = LLVMBuildLoad(builder, out, w->member.c_str());
  // std::cerr << LLVMPrintValueToString(baseinstr) << "\n";
  // std::cerr << LLVMPrintValueToString(out) << "\n";
  // std::cerr << "Not implemented yet: Member Codegen\n";
}

void coral::codegen::LLVMFunctionCompiler::visit(ast::While * w) {

  auto whileblock = LLVMAppendBasicBlock(function, "while");
  auto body = LLVMAppendBasicBlock(function, "do");
  auto endblock = LLVMAppendBasicBlock(function, "end");
  LLVMBuildBr(builder, whileblock);

  LLVMPositionBuilderAtEnd(builder, whileblock);
  out = 0;
  w->cond->accept(this);
  LLVMBuildCondBr(builder, out, body, endblock);

  LLVMPositionBuilderAtEnd(builder, body);
  returns=0;
  w->body->accept(this);
  if (!returns)
    LLVMBuildBr(builder, whileblock);

  LLVMPositionBuilderAtEnd(builder, endblock);
}
