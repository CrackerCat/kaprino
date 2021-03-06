#include <memory>
#include <vector>

#include "../../parser/KaprinoParserBaseVisitor.h"
#include "../abstructs/StatementObject.h"
#include "../KaprinoAccelerator.h"
#include "../StatementVisitor.h"

antlrcpp::Any StatementVisitor::visitProgram(KaprinoParser::ProgramContext *ctx) {
    auto statements = ctx->statement();

    auto statementsObj = new std::vector<StatementObject*>();

    KAPRINO_LOG("This program contains " << statements.size() << "statement(s)");

    for (auto statement : statements) {
        auto statementObjAny = visit(statement);

        auto statementObj = statementObjAny.as<StatementObject*>();
        statementsObj->push_back(statementObj);
    }

    return statementsObj;
}
