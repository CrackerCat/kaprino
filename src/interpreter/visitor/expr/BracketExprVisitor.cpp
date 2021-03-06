#include <stdlib.h>

#include "../../../parser/KaprinoParserBaseVisitor.h"
#include "../../abstructs/ExprObject.h"
#include "../../KaprinoAccelerator.h"
#include "../../StatementVisitor.h"

class BracketExprObject : ExprObject {
   public:
    ExprObject* value;

    virtual llvm::Value* codegen(llvm::IRBuilder<>* builder, llvm::Module* module) override {
        return value->codegen(builder, module);
    }
};

antlrcpp::Any StatementVisitor::visitBracketExpr(KaprinoParser::BracketExprContext* ctx) {
    auto exprObj = new BracketExprObject();

    exprObj->value = visit(ctx->expr()).as<ExprObject*>();

    return (ExprObject*)exprObj;
}
