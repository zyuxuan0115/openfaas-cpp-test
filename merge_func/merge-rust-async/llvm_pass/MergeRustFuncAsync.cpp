//===-- MergeRustFuncAsync.cpp - Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MergeRustFuncAsync.h"

using namespace llvm;

static cl::opt<bool> RenameCallee_rra(
                                     "rename-callee-rra", cl::init(false),
                                     cl::desc("rename the rust callee functions"));
static cl::opt<std::string> CalleeName_rra(
                                     "callee-name-rra", cl::Hidden,
                                     cl::desc("callee function name"),
                                     cl::init(""));

PreservedAnalyses MergeRustFuncAsyncPass::run(Module &M,
                                         ModuleAnalysisManager &AM) {
  if (CalleeName_rra=="") {
    llvm::errs()<<"[Error] Please set the callee function name\n";
    return PreservedAnalyses::all();
  }

  if (!RenameCallee_rra) {
    // get function::main::{{closure}}
    // because it contains RPC (OpenFaaSRPC::make_rpc())
    Function* mainClosure = getMainClosure(&M, CalleeName_rra);
    CallInst* rpcInst = getRPCinst(mainClosure, CalleeName_rra);

    // create a function that has the same arguments as `make_rpc`
    // but the function body is the callee function
    Function* CalleeFunc = M.getFunction("main_2nd_for_"+CalleeName_rra);
    Function* newCalleeFunc = cloneAndReplaceFuncWithDiffSignature(rpcInst, CalleeFunc, 
                                         "new_callee_"+CalleeName_rra);

    llvm::errs()<<"@@@@@@@@@@@\n";

    // get the callee function contains OpenFaaSRPC::get_arg_from_caller
    //Function* CalleeFaaSClosure = M.getFunction("faas_closure_for_"+CalleeName_rra);

    changeNewCalleeInput(newCalleeFunc);
    changeNewCalleeOutput(newCalleeFunc);

  }
  else {
    Function *mainFunc = M.getFunction("main");
    Function *rustRTFunc = getFunctionByDemangledName(&M, "std::rt::lang_start");
    //Function *faasClosure = getFaaSClousure(&M);
    renameCallee(mainFunc, "main_2nd_for_"+CalleeName_rra);
    mainFunc->setName("main_for_"+CalleeName_rra);
    rustRTFunc->setName("std_rt_lang_start_for_"+CalleeName_rra); 
    //faasClosure->setName("faas_closure_for_"+CalleeName_rra);
    //Function* mainClosure = getFunctionByDemangledName(&M, "function::main::{{closure}}");
    //if (mainClosure) {
    //  mainClosure->setName("callee_main_closure_for_"+CalleeName_rra);
    //}
  }
  return PreservedAnalyses::all();

}


Function* MergeRustFuncAsyncPass::getFaaSClousure(Module* M) {
  std::vector<Function*> funcs;
  for (Module::iterator f = M->begin(); f != M->end(); f++){
    Function* func = dyn_cast<Function>(f);
    std::string demangled = getDemangledRustFuncName(func->getName().str());
    if (demangled == "function::main") {
      funcs.push_back(func);
      llvm::errs()<<"@@@ "<<func->getName()<<"\n";
    }
  }

  for (auto f: funcs){
    for (Function::iterator BBB = f->begin(), BBE = f->end(); BBB != BBE; ++BBB){
      for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE; IB++){
        if (isa<CallInst>(IB)) {
          CallInst* ci = dyn_cast<CallInst>(IB);
          Function* calledFunc = ci->getCalledFunction();
          std::string calledFuncName = calledFunc->getName().str();
          std::string demangledName = getDemangledRustFuncName(calledFuncName);
          if (demangledName == "OpenFaaSRPC::get_arg_from_caller") {
            return f; 
          }
        }
      }
    } 
  }
  return NULL;
}



Function* MergeRustFuncAsyncPass::getMainClosure(Module* M, std::string callee_name){
  std::vector<Function*> main_funcs;
  for (Module::iterator f = M->begin(); f != M->end(); f++){
    Function* func = dyn_cast<Function>(f);
    std::string demangled = getDemangledRustFuncName(func->getName().str());
    if (demangled == "function::main::{{closure}}") {
      main_funcs.push_back(func);
    }
  }
  for (auto f: main_funcs) {
    for (Function::iterator BBB = f->begin(), BBE = f->end(); BBB != BBE; ++BBB){
      for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE; IB++){
        if (isa<CallInst>(IB)) {
          CallInst* ci = dyn_cast<CallInst>(IB);
          Function* calledFunc = ci->getCalledFunction();
          std::string calledFuncName = calledFunc->getName().str();
          std::string demangledName = getDemangledRustFuncName(calledFuncName);
          if (demangledName == "OpenFaaSRPC::make_rpc") {
            return f;
          } 
        }
      }
    }
  }
  return NULL;
}


CallInst* MergeRustFuncAsyncPass::getRPCinst(Function* f, std::string callee_name){
  for (Function::iterator BBB = f->begin(), BBE = f->end(); BBB != BBE; ++BBB){
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE; IB++){
      if (isa<CallInst>(IB)) {
        CallInst* ci = dyn_cast<CallInst>(IB);
        Function* calledFunc = ci->getCalledFunction();
        std::string calledFuncName = calledFunc->getName().str();
        std::string demangledName = getDemangledRustFuncName(calledFuncName);
        if (demangledName == "OpenFaaSRPC::make_rpc") {
          if (getRPCCalleeName(ci) == callee_name) {
            return ci;
          }
        } 
      }
    }
  }
  return NULL;
}




bool MergeRustFuncAsyncPass::IsStringStartWith(std::string str, std::string substr_start){
  if (str.size()<substr_start.size()) return false;
  if (str.rfind(substr_start, 0) == 0) return true;
  else return false;
}



std::string MergeRustFuncAsyncPass::getDemangledRustFuncName(std::string MangledFuncName) {
  std::string command = demangle_bin + " \'" + MangledFuncName + "\'";

  char* command_cstr = new char [command.length()+1];
  strcpy (command_cstr, command.c_str());

  FILE* fp1 = popen(command_cstr, "r");

  while (fp1 == NULL){
    sleep(1);
    fp1 = popen(command_cstr, "r");
    llvm::errs()<<"[tracer] fail to run demangle_rust_funcname\n";
    llvm::errs()<<command<<"\n";
  }

  char path1[3000];
  std::vector<std::string> lines;
  while (fgets(path1, sizeof(path1), fp1) != NULL) {
    std::string line(path1);
    lines.push_back(line);
  }
  pclose(fp1);
  if (lines.size()==1) { 
    return lines[0];
  }
  return "";
}



std::string MergeRustFuncAsyncPass::getRPCCalleeName(CallInst* RPCInst){
  Value* funcNameValue = RPCInst->getOperand(1);

  std::error_code EC;
  llvm::raw_fd_ostream output("tmp.txt", EC, sys::fs::OF_Text);

  funcNameValue->print(output);
  output.close();

  std::ifstream ifs;
  ifs.open("tmp.txt");
  std::stringstream oss;
  oss << ifs.rdbuf();
  std::string content = oss.str();
  ifs.close();
  std::vector<int> idxs;
  for (int i=0; i<content.size(); i++){
    if (content[i]=='"') idxs.push_back(i);
  }
  std::string fname = content.substr(idxs[0]+1,idxs[1]-idxs[0]-1); 
  return fname;
}




CallInst* MergeRustFuncAsyncPass::getCallByDemangledName(Function* f, std::string fname) {
  for (Function::iterator BBB = f->begin(), BBE = f->end(); BBB != BBE; ++BBB){
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE; IB++){
      if (isa<CallInst>(IB)){
        CallInst *ci = dyn_cast<CallInst>(IB);
        Function* CalledFunc = ci->getCalledFunction();
        std::string demangled = getDemangledRustFuncName(CalledFunc->getName().str());
        if (demangled == fname) return ci;
      }
    }
  }
  return NULL; 
}



std::vector<CallInst*> MergeRustFuncAsyncPass::getCallsByDemangledName(Function* f, std::string fname){
  std::vector<CallInst*> calls;
  for (Function::iterator BBB = f->begin(), BBE = f->end(); BBB != BBE; ++BBB){
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE; IB++){
      if (isa<CallInst>(IB)){
        CallInst *ci = dyn_cast<CallInst>(IB);
        Function* CalledFunc = ci->getCalledFunction();
        std::string demangled = getDemangledRustFuncName(CalledFunc->getName().str());
        if (demangled == fname) calls.push_back(ci);
      }
    }
  } 
  return calls;
}



InvokeInst* MergeRustFuncAsyncPass::getInvokeByDemangledName(Function* f, std::string fname) {
  for (Function::iterator BBB = f->begin(), BBE = f->end(); BBB != BBE; ++BBB){
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE; IB++){
      if (isa<InvokeInst>(IB)) {
        InvokeInst* ii = dyn_cast<InvokeInst>(IB);
        std::string demangled = getDemangledRustFuncName(ii->getCalledFunction()->getName().str());
        if (demangled == fname) return ii; 
      } 
    }
  }
  return NULL; 
}




Function* MergeRustFuncAsyncPass::getFunctionByDemangledName(Module* M, std::string fname) {
  for (Module::iterator f = M->begin(); f != M->end(); f++){
    Function* func = dyn_cast<Function>(f);
    std::string demangled = getDemangledRustFuncName(func->getName().str());
    if (demangled == fname) return func;
  }
  return NULL;
}




// rename the callee function (main) to be "callee"
void MergeRustFuncAsyncPass::renameCallee(Function* mainFunc, std::string newCalleeName){
  Function *rustRTFunc; 

  for (Function::iterator BBB = mainFunc->begin(), BBE = mainFunc->end(); BBB != BBE; ++BBB){
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE; IB++){
      if(isa<CallInst>(IB)){
        CallInst *ci = dyn_cast<CallInst>(IB);
        Function* realMainFunc = dyn_cast<Function>(ci->getArgOperand(0));
        rustRTFunc = ci->getCalledFunction();
	realMainFunc->setName(newCalleeName.c_str());
      }
    }
  }
  return;    
}



Function* MergeRustFuncAsyncPass::cloneAndReplaceFunc(CallInst* callOfTargetFunc, std::string newFuncName) {
  // copy this function
  std::vector<Value*> arguments;
  std::vector<Type*> argumentTypes;
  CallInst* call = callOfTargetFunc;
  Function* targetFunc = call->getCalledFunction();
  Module* M = targetFunc->getParent();

  // the last argument of a CallInst is the called function
  // so we don't need to include it in the arguments.
  for (unsigned i=0; i<call->getNumOperands()-1; i++){
    Value* arg = call->getOperand(i);
    arguments.push_back(arg);
    argumentTypes.push_back(arg->getType());
  }

  FunctionType* FuncType = targetFunc->getFunctionType();
  Function * newFunc = Function::Create(FuncType, llvm::GlobalValue::ExternalLinkage, newFuncName, M);
  ValueToValueMapTy VMap;
  SmallVector<ReturnInst*, 8> Returns;

  Function::arg_iterator DestI = newFunc->arg_begin();
  for (const Argument &J : targetFunc->args()) {
    DestI->setName(J.getName());
    VMap[&J] = &*DestI++;
  }
 
  CloneFunctionInto(newFunc, targetFunc, VMap, llvm::CloneFunctionChangeType::LocalChangesOnly, Returns);

  // set attributes for the new callee function's arguments
  std::vector<AttributeSet> argumentAttrs;
  AttributeList AttrList = targetFunc->getAttributes();
  for (unsigned i=0; i<arguments.size(); i++){
    argumentAttrs.push_back(AttrList.getParamAttrs(i));
  }

  AttributeList newAttrList = targetFunc->getAttributes();
  AttributeSet returnAttr = newAttrList.getRetAttrs();
  AttributeSet funcAttr = newAttrList.getFnAttrs();

  newFunc->setAttributes(AttributeList::get(M->getContext(), funcAttr, returnAttr, argumentAttrs));

  // copy the old future_maybe function  
  CallInst* newCall = CallInst::Create(FuncType, newFunc, arguments ,"", call);
  AttributeList callInstAttr = call->getAttributes();
  newCall->setAttributes(callInstAttr);

  // get the user of callInst for calling old future_maybe 
  for (auto u = call->user_begin(); u!= call->user_end(); u++) {
    User* user = dyn_cast<User>(*u);
    for (auto oi = user->op_begin(); oi != user->op_end(); oi++) {
      Value *val = *oi;
      Value *call_value = dyn_cast<Value>(call);
      if (val == call_value){
        *oi = dyn_cast<Value>(newCall);
      }
    }
  }
  call->eraseFromParent();
  
  return newFunc;
}



Function* MergeRustFuncAsyncPass::cloneAndReplaceFuncWithDiffSignature(CallInst* call, Function* targetFunc, std::string newFuncName) {
  Module* M = targetFunc->getParent();
  Function* originalCalledFunc = call->getCalledFunction();
  std::vector<Value*> arguments;
  std::vector<Type*> argumentTypes;

  for (unsigned i=0; i<call->getNumOperands()-1; i++){
    Value* arg = call->getOperand(i);
    arguments.push_back(arg);
    argumentTypes.push_back(arg->getType());
  }
  FunctionType* FuncType = originalCalledFunc->getFunctionType();
  Function * newCalleeFunc = Function::Create(FuncType, llvm::GlobalValue::ExternalLinkage, newFuncName, M);
  // update VMap, the targetFunc has less args than the original callee function
  // so we need to remap the arguments
  ValueToValueMapTy VMap;
  SmallVector<ReturnInst*, 8> Returns;

  CloneFunctionInto(newCalleeFunc, targetFunc, VMap, llvm::CloneFunctionChangeType::LocalChangesOnly, Returns);

  // set attributes for the new callee function's arguments
  std::vector<AttributeSet> argumentAttrs;
  AttributeList newCalleeAttrList = originalCalledFunc->getAttributes();
  for (unsigned i=0; i<arguments.size(); i++){
    argumentAttrs.push_back(newCalleeAttrList.getParamAttrs(i));
  }
  AttributeSet returnAttr = newCalleeAttrList.getRetAttrs();
  AttributeSet funcAttr = newCalleeAttrList.getFnAttrs();

  newCalleeFunc->setAttributes(AttributeList::get(M->getContext(), funcAttr, returnAttr, argumentAttrs));

  // create a new before the OpenFaaS::make_rpc (call) that 
  // points to the new function
  CallInst* newCall = CallInst::Create(FuncType, newCalleeFunc, arguments ,"", call);
  AttributeList attr = call->getAttributes();
  newCall->setAttributes(attr);
  call->eraseFromParent();

  return newCalleeFunc;
}




void MergeRustFuncAsyncPass::changeNewCalleeOutput(Function* newCalleeFunc) {
  Module* M = newCalleeFunc->getParent();
  InvokeInst* send_return_value_call = getInvokeByDemangledName(newCalleeFunc, 
       "OpenFaaSRPC::send_return_value_to_caller");

  // create call void @llvm.memcpy.p0.p0.i64(ptr align 8 %_0, 
  //                                         ptr align 8 %buffer, 
  //                                         i64 24, i1 false)
  // the is the LLVM Intrinsc. The way to create such a call 
  // is different from normal CallInst create 

  std::vector<Type*> IntrinTypes;
  IntrinTypes.push_back(newCalleeFunc->getArg(0)->getType());
  IntrinTypes.push_back(send_return_value_call->getOperand(0)->getType());
  IntrinTypes.push_back(Type::getInt64Ty(M->getContext()));

  Constant* i64_24 = llvm::ConstantInt::get(Type::getInt64Ty(M->getContext()), 24, true);
  Constant* i1_false = llvm::ConstantInt::get(Type::getInt1Ty(M->getContext()), 0, true);
  Function* llvmMemcpyFunc = Intrinsic::getDeclaration(M, Intrinsic::memcpy, IntrinTypes);

  std::vector<Value*> IntrinsicArguments;
  IntrinsicArguments.push_back(newCalleeFunc->getArg(0));
  IntrinsicArguments.push_back(send_return_value_call->getOperand(0));
  IntrinsicArguments.push_back(dyn_cast<Value>(i64_24));
  IntrinsicArguments.push_back(dyn_cast<Value>(i1_false));
  ArrayRef<Value*> IntrinsicArgs(IntrinsicArguments);


  IRBuilder<> Builder(M->getContext());
  CallInst* llvmMemcpyCall = Builder.CreateCall(llvmMemcpyFunc, IntrinsicArgs);
  llvmMemcpyCall->insertBefore(send_return_value_call);

  // since the instruction is a InvokeInst, also needs to create a branch instruction
  BasicBlock* nextBBofsend_ret_value_call = dyn_cast<BasicBlock>(send_return_value_call->getOperand(1));
  if (nextBBofsend_ret_value_call)
    BranchInst * jumpInst = llvm::BranchInst::Create(nextBBofsend_ret_value_call, send_return_value_call);
  send_return_value_call->eraseFromParent();
} 




void MergeRustFuncAsyncPass::changeNewCalleeInput(Function* newCalleeFunc) {
  Module* M = newCalleeFunc->getParent();
  CallInst* get_arg_call = getCallByDemangledName(newCalleeFunc,
     "OpenFaaSRPC::get_arg_from_caller");

  // in the new function, also need to change the way of how input arguments are get
  // (1) first need to check the user of the existing function arguments
  //     and delete all instructions that depends on these arguments
  std::vector<Value*> args_of_new_callee;
  for (Argument &ag : newCalleeFunc->args()) {
    args_of_new_callee.push_back(&ag);
    llvm::errs()<<"@@@ "<<ag<<"\n";
  }
  Value* func_arg = args_of_new_callee[3];
/*
  Value* alloc_of_arg;
  for (auto u = arg_matters->user_begin(); u!= arg_matters->user_end(); u++) {
    Instruction* user = dyn_cast<Instruction>(*u);
    if (isa<StoreInst>(user)) {
      alloc_of_arg = user->getOperand(1);
    }
  }
*/
  // (3) insert a LoadInst before the get_arg_call
//  LoadInst* newload = new LoadInst(arg_matters->getType(), alloc_of_arg, "", get_arg_call);

  Value* arg_input = get_arg_call->getOperand(0);
//  Instruction* arg_input2 = dyn_cast<Instruction>(arg_input)->clone();
//  arg_input2->insertBefore(dyn_cast<Instruction>(arg_input));

/*
  for (auto u = arg_input->user_begin(); u!= arg_input->user_end(); u++) {
    Value* user = dyn_cast<User>(*u);
    if (isa<InvokeInst>(user)) {
      llvm::errs()<<*user<<"\n";
      InvokeInst* invoke = dyn_cast<InvokeInst>(user);
      std::string demangled = getDemangledRustFuncName(invoke->getCalledFunction()->getName().str());
      if (demangled=="core::ptr::drop_in_place<alloc::string::String>") {
        for (auto oi = invoke->op_begin(); oi!=invoke->op_end(); oi++){
          Value* operand = dyn_cast<Value>(*oi);
          if (operand == arg_input) {
            *oi = dyn_cast<Value>(arg_input2);
          }
        } 
      }
    }
  }
*/
  IRBuilder<> Builder(M->getContext());

  Constant* i64_24 = llvm::ConstantInt::get(Type::getInt64Ty(M->getContext()), 24, true);
  Constant* i1_false = llvm::ConstantInt::get(Type::getInt1Ty(M->getContext()), 0, true);

  std::vector<Type*> IntrinTypes;
  IntrinTypes.push_back(get_arg_call->getOperand(0)->getType());
  IntrinTypes.push_back(func_arg->getType());
  IntrinTypes.push_back(Type::getInt64Ty(M->getContext()));

  Function* llvmMemcpyFunc = Intrinsic::getDeclaration(M, Intrinsic::memcpy, IntrinTypes); 

  std::vector<Value*> IntrinsicArguments;
  IntrinsicArguments.push_back(get_arg_call->getOperand(0));
  IntrinsicArguments.push_back(func_arg);
  IntrinsicArguments.push_back(dyn_cast<Value>(i64_24));
  IntrinsicArguments.push_back(dyn_cast<Value>(i1_false));
  ArrayRef<Value*> IntrinsicArgs(IntrinsicArguments);

  CallInst* llvmMemcpyCall = Builder.CreateCall(llvmMemcpyFunc, IntrinsicArgs);
  llvmMemcpyCall->insertBefore(get_arg_call);

  BasicBlock* nextBB2 = dyn_cast<BasicBlock>(get_arg_call->getOperand(1));
  if (nextBB2) {
    BranchInst * jumpInst = llvm::BranchInst::Create(nextBB2, get_arg_call); 
    get_arg_call->eraseFromParent();
  }
}
