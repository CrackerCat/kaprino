#include <memory>
#include <vector>

#include "../../../parser/KaprinoParserBaseVisitor.h"
#include "../../abstructs/ExprObject.h"
#include "../../abstructs/FunctionTypeObject.h"
#include "../../abstructs/StatementObject.h"
#include "../../FunctionManager.h"
#include "../../KaprinoAccelerator.h"
#include "../../StatementVisitor.h"
#include "../../TypeManager.h"
#include "../../VariableManager.h"

class DefineProcessStatementObject : StatementObject {
   public:
    std::string funcName;
    std::vector<std::string> inputParamNames;
    FunctionTypeObject* functionType;
    std::vector<StatementObject*>* statements;

    virtual void codegen(llvm::IRBuilder<>* builder, llvm::Module* module) override {
        auto previousblock = builder->GetInsertBlock();
        VariableManager::add_scope();
        {
            auto types = functionType->inputTypes;
            std::vector<llvm::Type*> inputTypes;
            for (auto type : types) {
                inputTypes.push_back(TypeManager::gettype(builder, module, type));
            }

            auto userdefinedType = llvm::FunctionType::get(
                TypeManager::gettype(builder, module, functionType->outputType),
                inputTypes,
                true
            );

            auto userdefinedFunc = llvm::Function::Create(
                userdefinedType,
                llvm::GlobalValue::ExternalLinkage,
                funcName,
                module
            );
            userdefinedFunc->setCallingConv(llvm::CallingConv::C);

            auto funcBody = llvm::BasicBlock::Create(module->getContext(), "", userdefinedFunc);
            builder->SetInsertPoint(funcBody);

            int size = inputParamNames.size();
            std::vector<llvm::Value*> args;
            for (auto& arg : userdefinedFunc->args()) {
                args.push_back(&arg);
            }
            for (int counter = 0; counter < size; counter++) {
                auto arg = args[counter];
                auto allocated = builder->CreateAlloca(arg->getType());
                builder->CreateStore(arg, allocated);
                VariableManager::create(
                    builder,
                    module,
                    inputParamNames[counter],
                    allocated
                );
            }

            for (auto st : *statements) {
                st->codegen(builder, module);
            }

            FunctionManager::create(builder, module, funcName, userdefinedFunc);
        }
        VariableManager::remove_scope();
        builder->SetInsertPoint(previousblock);
    }
};

antlrcpp::Any StatementVisitor::visitDefineProcessStatement(KaprinoParser::DefineProcessStatementContext* ctx) {
    auto statementObj = new DefineProcessStatementObject();

    auto names = ctx->ID();
    statementObj->funcName = names[0]->getText();
    int size = names.size();
    for(int counter = 1; counter < size; counter++) {
        statementObj->inputParamNames.push_back(names[counter]->getText());
    }
    statementObj->functionType = visit(ctx->function_type()).as<FunctionTypeObject*>();
    statementObj->statements = visit(ctx->codeblock()).as<std::vector<StatementObject*>*>();

    return (StatementObject*)statementObj;
}
