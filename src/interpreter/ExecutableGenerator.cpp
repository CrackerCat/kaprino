#include <ostream>
#include <sstream>
#include <string>

#include "ExecutableGenerator.h"
#include "KaprinoAccelerator.h"

void EmitLLVMIR(llvm::Module* module, bool optimize) {
    KAPRINO_LOG("Emit LLVM IR file to: " << module->getName().str());

    std::error_code errorcode;
    auto stream = new llvm::raw_fd_ostream(module->getName(), errorcode);

    module->print(*stream, nullptr);

    KAPRINO_LOG("Outputing file task: " << errorcode.message());
}

std::string EmitObjectCode(llvm::Module* module, bool optimize) {
    auto llvmir_path = module->getName().str();

#if _WIN32

    auto objectcode_path = KAPRINO_RM_FILE_EXT(llvmir_path) + ".obj";

#else

    auto objectcode_path = KAPRINO_RM_FILE_EXT(llvmir_path) + ".o";

#endif

    remove(objectcode_path.c_str());

    auto target_triple = llvm::sys::getDefaultTargetTriple();
    module->setTargetTriple(target_triple);

    std::string errormsg;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, errormsg);

    if (!target) {
        KAPRINO_ERR(errormsg);
        throw -1;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto target_machine = target->createTargetMachine(
        target_triple, CPU, Features, opt, RM, llvm::None,
        optimize ? llvm::CodeGenOpt::Aggressive : llvm::CodeGenOpt::Default
    );

    module->setDataLayout(target_machine->createDataLayout());

    std::error_code error_code;
    llvm::raw_fd_ostream dest(objectcode_path, error_code, llvm::sys::fs::OF_None);

    if (error_code) {
        KAPRINO_ERR("Object code generation faield: " << error_code);
        throw -1;
    }

    llvm::legacy::PassManager pass;

    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, llvm::TargetMachine::CGFT_ObjectFile)) {
        KAPRINO_ERR("Object code generation faield");
        throw -1;
    }

    pass.run(*module);
    dest.flush();

    return objectcode_path;
}

std::string EmitExecutable(llvm::Module* module, bool optimize) {
    auto objectcode_path = EmitObjectCode(module, optimize);

#if _WIN32

    auto executable_path = KAPRINO_RM_FILE_EXT(module->getName().str()) + ".exe";

#else

    auto executable_path = KAPRINO_RM_FILE_EXT(module->getName().str());

#endif

    std::ostringstream compile_command;
    compile_command << "clang -o ";
    compile_command << executable_path;

#if _WIN32

    compile_command << " -llegacy_stdio_definitions.lib ";

#endif

    compile_command << " ";
    compile_command << objectcode_path;

    KAPRINO_LOG("Execute external tool: " << compile_command.str());

    auto error_code = system(compile_command.str().c_str());
    if (error_code) {
        KAPRINO_ERR("Clang faield: " << error_code);
        throw error_code;
    }

    KAPRINO_LOG("Compile complete");

    return executable_path;
}
