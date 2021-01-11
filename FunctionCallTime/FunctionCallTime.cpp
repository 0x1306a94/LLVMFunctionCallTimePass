//
//  function_call_time.cpp
//  function-call-time
//
//  Created by king on 2020/12/18.
//

#include <iostream>

#include "llvm/ADT/Statistic.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;
namespace {
struct FunctionCallTimePass : public FunctionPass {
	static char ID;

	static StringRef InsertFuncNamePrefix;
	static StringRef BeginFuncName;
	static StringRef EndFuncName;

	FunctionCallTimePass()
	    : FunctionPass(ID) {}

	bool runOnFunction(Function &F) override {

		if (F.empty()) {
			return false;
		}
		std::string annotation = readAnnotate(&F);

		auto funcName = F.getName();
		// 可以通过 __attribute__((__annotate__(("ignore_appletrace"))))
		// 忽略当前函数
		if (annotation.length() > 0 &&
		    StringRef(annotation).contains("ignore_appletrace")) {
			errs() << "function-call-time: " << funcName << " 忽略 annotation"
			       << "\n";
			return false;
		}
		// Objective-C 方法前面有 \x01
		if (funcName.front() == '\x01') {
			funcName = funcName.drop_front();
		}

		if (funcName.startswith("__Z") || funcName.startswith("_Z")) {
			// C++ 函数
			std::string str       = funcName.str();
			std::string demangled = demangle(str);
			funcName              = StringRef(demangled);
		}
		// 将统计代码调用过滤掉
		if (funcName.startswith("appletrace")) {
			return false;
		}
		// 如果是插桩的函数直接跳过
		if (F.getName().startswith(FunctionCallTimePass::InsertFuncNamePrefix)) {
			return false;
		}

		// 只统计 Objective-C 方法调用
		if (funcName.startswith("+[") || funcName.startswith("-[")) {
			// 2. 插入开始
			if (!insertBeginInst(F)) {
				return false;
			}
			// 3. 插入结束
			insertEndInst(F);
			return false;
		}
		return false;
	}

  private:
	std::string readAnnotate(Function *f) {
		std::string annotation = "";

		// Get annotation variable
		GlobalVariable *glob =
		    f->getParent()->getGlobalVariable("llvm.global.annotations");

		if (glob != NULL) {
			// Get the array
			if (ConstantArray *ca = dyn_cast<ConstantArray>(glob->getInitializer())) {
				for (unsigned i = 0; i < ca->getNumOperands(); ++i) {
					// Get the struct
					if (ConstantStruct *structAn =
					        dyn_cast<ConstantStruct>(ca->getOperand(i))) {
						if (ConstantExpr *expr =
						        dyn_cast<ConstantExpr>(structAn->getOperand(0))) {
							// If it's a bitcast we can check if the annotation is concerning
							// the current function
							if (expr->getOpcode() == Instruction::BitCast &&
							    expr->getOperand(0) == f) {
								ConstantExpr *note =
								    cast<ConstantExpr>(structAn->getOperand(1));
								// If it's a GetElementPtr, that means we found the variable
								// containing the annotations
								if (note->getOpcode() == Instruction::GetElementPtr) {
									if (GlobalVariable *annoteStr =
									        dyn_cast<GlobalVariable>(note->getOperand(0))) {
										if (ConstantDataSequential *data =
										        dyn_cast<ConstantDataSequential>(
										            annoteStr->getInitializer())) {
											if (data->isString()) {
												annotation += data->getAsString().lower() + " ";
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return annotation;
	}

	bool insertBeginInst(Function &F) {
		// 0.函数最开始的BasicBlock
		LLVMContext &context = F.getParent()->getContext();
		BasicBlock &BB       = F.getEntryBlock();

		// 1. 获取要插入的函数
		FunctionCallee beginFun = F.getParent()->getOrInsertFunction(
		    FunctionCallTimePass::BeginFuncName,
		    FunctionType::get(Type::getVoidTy(context),
		                      {Type::getInt8PtrTy(context)}, false));

        auto funcName = BB.getParent()->getName();
        // Objective-C 方法前面有 \x01
        if (funcName.front() == '\x01') {
            funcName = funcName.drop_front();
        }

        if (funcName.startswith("__Z") || funcName.startswith("_Z")) {
            // C++ 函数
            std::string str       = funcName.str();
            std::string demangled = demangle(str);
            funcName              = StringRef(demangled);
        }
        
		errs() << "function-call-time: " << funcName << " begin\n";
		// 2. 构造函数
		CallInst *inst = nullptr;
		IRBuilder<> builder(&BB);
		IRBuilder<> callBuilder(context);
		Value *name = builder.CreateGlobalStringPtr(BB.getParent()->getName());
		inst        = callBuilder.CreateCall(beginFun, {name});

		if (!inst) {
			llvm::errs() << "Create First CallInst Failed\n";
			return false;
		}

		// 3. 获取函数开始的第一条指令
		Instruction *beginInst = dyn_cast<Instruction>(BB.begin());

		// 4. 将inst插入
		inst->insertBefore(beginInst);

		return true;
	}

	void insertEndInst(Function &F) {
		LLVMContext &context = F.getParent()->getContext();
		for (Function::iterator I = F.begin(), E = F.end(); I != E; ++I) {

			//  函数结尾的BasicBlock
			BasicBlock &BB = *I;
			for (BasicBlock::iterator I = BB.begin(), E = BB.end(); I != E; ++I) {
				ReturnInst *IST = dyn_cast<ReturnInst>(I);
				if (!IST)
					continue;

				// end_func 类型
				FunctionType *endFuncType = FunctionType::get(
				    Type::getVoidTy(context), {Type::getInt8PtrTy(context)}, false);

				// end_func
				FunctionCallee endFunc = BB.getModule()->getOrInsertFunction(
				    FunctionCallTimePass::EndFuncName, endFuncType);

				// 构造end_func
				IRBuilder<> builder(&BB);
				IRBuilder<> callBuilder(context);
				Value *name     = builder.CreateGlobalStringPtr(BB.getParent()->getName());
				CallInst *endCI = callBuilder.CreateCall(endFunc, {name});

				// 插入end_func(struction)
				endCI->insertBefore(IST);
                
                auto funcName = BB.getParent()->getName();
                // Objective-C 方法前面有 \x01
                if (funcName.front() == '\x01') {
                    funcName = funcName.drop_front();
                }

                if (funcName.startswith("__Z") || funcName.startswith("_Z")) {
                    // C++ 函数
                    std::string str       = funcName.str();
                    std::string demangled = demangle(str);
                    funcName              = StringRef(demangled);
                }

				errs() << "function-call-time: " << funcName
				       << " end\n";
			}
		}
	}
};
}  // namespace

char FunctionCallTimePass::ID                        = 0;
StringRef FunctionCallTimePass::InsertFuncNamePrefix = "_kk_APT";
StringRef FunctionCallTimePass::BeginFuncName        = "_kk_APTBeginSection";
StringRef FunctionCallTimePass::EndFuncName          = "_kk_APTEndSection";
// 注册给 opt
// opt -load LLVMFunctionCallTime.dylib -function-call-time xx.bc
static RegisterPass<FunctionCallTimePass>
    X("function-call-time", "Function calls take time to collect", false,
      false);
// 注册给 clang 通过 -Xclang -load -Xclang LLVMFunctionCallTime.dylib
static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
	                                PM.add(new FunctionCallTimePass());
                                });

