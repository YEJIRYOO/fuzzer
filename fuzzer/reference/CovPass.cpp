// =============================================================================
// REFERENCE solution for pass/CovPass.cpp.  Instructor-only.
//
// Drop this on top of pass/CovPass.cpp to verify the lab is wired up.
// =============================================================================

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <random>

using namespace llvm;

namespace {

static std::mt19937 gRng(0xC0FFEE);

struct CovPass : PassInfoMixin<CovPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
        LLVMContext &Ctx = M.getContext();
        FunctionCallee VisitFn = M.getOrInsertFunction(
            "__cov_visit",
            FunctionType::get(Type::getVoidTy(Ctx),
                              {Type::getInt32Ty(Ctx)}, false));

        unsigned bbs = 0, fns = 0;
        for (Function &F : M) {
            if (F.isDeclaration()) continue;
            if (F.getName() == "__cov_visit") continue;
            ++fns;
            for (BasicBlock &BB : F) {
                IRBuilder<> IRB(&*BB.getFirstInsertionPt());
                uint32_t id = (uint32_t)(gRng() & 0xFFFF);
                IRB.CreateCall(VisitFn, {IRB.getInt32(id)});
                ++bbs;
            }
        }
        errs() << "[CovPass] instrumented " << bbs
               << " basic blocks across " << fns
               << " functions in module " << M.getName() << "\n";
        return PreservedAnalyses::none();
    }

    static bool isRequired() { return true; }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "CovPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel) {
                    MPM.addPass(CovPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "cov-pass") {
                        MPM.addPass(CovPass());
                        return true;
                    }
                    return false;
                });
        }};
}
