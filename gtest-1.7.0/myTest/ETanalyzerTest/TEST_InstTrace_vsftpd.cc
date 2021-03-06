#include "gtest/gtest.h"
//for ET-analyzer
#include "/home/jun/llvm-3.x/llvm/lib/Transforms/Hello/common.h"
#include "/home/jun/llvm-3.x/llvm/lib/Transforms/Hello/DEBUG_utils.h"
#include "/home/jun/llvm-3.x/llvm/lib/Transforms/Hello/INPUT_SigHandler.h"
#include "/home/jun/llvm-3.x/llvm/lib/Transforms/Hello/INPUT_GlobalData.h"
#include "/home/jun/llvm-3.x/llvm/lib/Transforms/Hello/INPUT_InstTrace.h"
#include "/home/jun/llvm-3.x/llvm/lib/Transforms/Hello/CallInstClass.h"
//for load module from file
#include </home/jun/llvm-3.x/llvm/include/llvm/IR/Module.h>
#include </home/jun/llvm-3.x/llvm/include/llvm/IRReader/IRReader.h>
#include </home/jun/llvm-3.x/llvm/include/llvm/IR/LLVMContext.h>
#include </home/jun/llvm-3.x/llvm/include/llvm/Support/SourceMgr.h>

#include<vector>
#include<algorithm>

using namespace std;
using namespace llvm;

shared_ptr<INPUT_GlobalData> globalData;
shared_ptr<INPUT_SigHandlerSet> sigHandlerSet;
shared_ptr<DEBUG_utils> debug_utils;

//#define DEBUGTRACE
//declare test fixture
class InstTraceFix: public testing::Test{
  protected:   
    virtual void SetUp(){
      string irfile_vsftpd("/home/jun/straight-DTA/vsftpd-2.2.2/test/vsftpd.bc");
      m2=ParseIRFile(irfile_vsftpd.c_str(), error2, context2);
		}
    LLVMContext context2;
    SMDiagnostic error2;
    Module const * m2;
};
/*
TEST_F(InstTraceFix, ForkQueue_vsftpd){
  string logdir("/home/jun/straight-DTA/vsftpd-2.2.2/test");
  string cfigfile("configFile");

  debug_utils.reset(new DEBUG_utils());
  sigHandlerSet.reset(new INPUT_SigHandlerSet( (logdir+"/"+"sigHandlerFile").c_str() ));
  globalData.reset(new INPUT_GlobalData(*m2));

  INPUT_InstTrace* it=new INPUT_InstTrace( *m2, logdir, cfigfile );
  EXPECT_FALSE(it->isForkQueueEmpty());
  char* fq1=it->getForkQueueFront();
  EXPECT_TRUE(fq1!=NULL);
  string fq1s(fq1);
  EXPECT_TRUE(fq1s == "tmp.27340.full");
  it->popForkQueue();
  EXPECT_FALSE(it->isForkQueueEmpty());
  char* fq2=it->getForkQueueFront();
  EXPECT_TRUE(fq2!=NULL);
  string fq2s(fq2);
  EXPECT_TRUE(fq2s == "tmp.27348.full");
  it->popForkQueue();
  EXPECT_TRUE(it->isForkQueueEmpty());

  delete it;

  debug_utils.reset((DEBUG_utils*)NULL);
  sigHandlerSet.reset((INPUT_SigHandlerSet*)NULL);
  globalData.reset((INPUT_GlobalData*)NULL);
}
*/
bool isEntryInst(Instruction const* inst){
	if(inst==NULL)return false;
	BasicBlock const* bbl=inst->getParent();
	if(bbl==NULL)return false;
	if(&(bbl->front())!=inst)return false;
	Function const* func=bbl->getParent();
	if(func==NULL)return false;
	if(bbl!=&(func->getEntryBlock()))return false;
	return true;
}

bool isCallExit(Instruction const* inst){
	if(CallInst const* callInst=dyn_cast<CallInst>(inst)){
		Function const* func=callInst->getCalledFunction();
		if(func==NULL)return false;
		string funcName(func->getName().data());
		if(funcName=="exit"){
			return true;
		}
		else{
			return false;
		}
	}else{
		return false;
	}
}

void dumpInstTrace(deque<Instruction const*> const instTrace){
	for(auto i=instTrace.begin(), i_e=instTrace.end(); i!=i_e; i++){
		Instruction const* inst=*i;
		if(BasicBlock const* bbl=inst->getParent()){
			cerr<<bbl->getParent()->getName().data()<<"\t";
		}
		inst->dump();
	}
}

bool verifyInstTrace(deque<Instruction const*> const instTrace){
	stack<Function const*> callstk;
	auto i=instTrace.begin(), i_e=instTrace.end();
	auto i_last=i;
	auto i_b=i;
	for(; i!=i_e; i++){
		Instruction const* inst=*i;
		if(i!=i_b && i!=i_b+1){
      i_last++;
		}
		if(isEntryInst(inst)){
		  if(i!=i_b){
        auto inst_last=*i_last;
        if(isa<CallInst>(inst_last)==false){
          cerr<<"last instruction: ";
          inst_last->dump();
          cerr<<"last instruction is in function: "<<inst_last->getParent()->getParent()->getName().data()<<"\n";
          cerr<<"Function name of the current function: "<<inst->getParent()->getParent()->getName().data()<<"\n";
          cerr<<"last instruction of a entry inst is not a callInst\n";
          EXPECT_FALSE(true);
        }
		  }
			Function const* func=inst->getParent()->getParent();
			callstk.push(func);
		}else if(isa<ReturnInst>(inst) || isCallExit(inst)){
			if(callstk.empty()){
				cout<<"InstTrace Error: Excessive return inst!\n";
				return false;
			}else{
				Function const* tmp=callstk.top();
				if(tmp==inst->getParent()->getParent()){
					callstk.pop();
				}else{
					cout<<"InstTrace Error: call-return not match!\n";
					cout<<"Entry function: "<<tmp->getName().data()<<"\n";
					cout<<"ReturnInst in function: "<<inst->getParent()->getParent()->getName().data()<<"\n";
					return false;
				}
			}
		}
		
	}
	return true;
}

bool verifyCallRet(deque<Instruction const*> const InstTrace){
  stack<CallInstClass> callStk;
  for(auto i=InstTrace.begin(), i_e=InstTrace.end(); i!=i_e; i++){
    Instruction const* inst=*i;
    if(CallInst const* callInst=dyn_cast<CallInst>(inst)){
      CallInstClass cic(callInst);
      if(cic.isDeterministicInternalCall() || cic.isPseudoCall()){
#ifdef DEBUGTRAE
        inst->dump();
#endif
        callStk.push(cic);
      }
    }else if(isa<ReturnInst>(inst)==true){
#ifdef DEBUGTRAE
        inst->dump();
#endif
      if(callStk.empty()){
        if(i+1==i_e){
          continue;
        }
        cerr<<"Excessive ret!\n";
        cerr<<"Function of returnInst: "<<inst->getParent()->getParent()->getName().data()<<"\n";
        EXPECT_TRUE(false);
        return false;
      }
      string funcNameOfInst=inst->getParent()->getParent()->getName().data();
      CallInstClass & cic=callStk.top();
      string funcNameOfCall=cic.getCalleeName();
      if(funcNameOfCall=="" || funcNameOfCall==funcNameOfInst || cic.isPseudoCall()){
        callStk.pop();
      }else{
        cerr<<"Call-ret not match.\n";
        cerr<<"funcNameOfCall: "<<funcNameOfCall<<"\n";
        cerr<<"funcNameOfRet: "<<funcNameOfInst<<"\n";
        EXPECT_TRUE(false);
      }
    }else if(isa<UnreachableInst>(inst)){
#ifdef DEBUGTRAE
        inst->dump();
#endif
      if(i+1==i_e){
        continue;
      }
      cerr<<"Inst trace not finished after UnreachableInst.\n";
      EXPECT_TRUE(false);
    }
  }
  return true;
}

TEST_F(InstTraceFix, traceBB_vsftpd){
	string logdir("/home/jun/straight-DTA/vsftpd-2.2.2/test");
	string cfigfile("configFile");

	debug_utils.reset(new DEBUG_utils());
	sigHandlerSet.reset(new INPUT_SigHandlerSet( (logdir+"/"+"sigHandlerFile").c_str() ));
	globalData.reset(new INPUT_GlobalData(*m2));

	INPUT_InstTrace* it=new INPUT_InstTrace( *m2, logdir, cfigfile );
	EXPECT_FALSE(it->isForkQueueEmpty());
	//auto traceIsValid=verifyInstTrace(it->getTraceInstForReadOnly());
	//EXPECT_TRUE(traceIsValid);
	auto callretIsValid=verifyCallRet(it->getTraceInstForReadOnly());
	EXPECT_TRUE(callretIsValid);
#ifdef DEBUGTRAE
	cerr<<"\nChild process trace: \n";
#endif
	//child process
	char const* traceFileNameChild=it->getLogFileName();
	it->popForkQueue();
	EXPECT_FALSE(it->isForkQueueEmpty());
	char const* traceFileNameChild2=it->getLogFileName();
	it->updateInstTrace(traceFileNameChild);
	EXPECT_FALSE(it->isForkQueueEmpty());
	//traceIsValid=verifyInstTrace(it->getTraceInstForReadOnly());
  //EXPECT_TRUE(traceIsValid);
  callretIsValid=verifyCallRet(it->getTraceInstForReadOnly());
  EXPECT_TRUE(callretIsValid);
	//child of child process
	char const* traceFileNameChildOfChild=it->getLogFileName();
	it->popForkQueue();
	char const* traceFileNameChild2OfChild=it->getLogFileName();
	it->updateInstTrace(traceFileNameChildOfChild);
	EXPECT_TRUE(it->isForkQueueEmpty());
	//traceIsValid=verifyInstTrace(it->getTraceInstForReadOnly());
  //EXPECT_TRUE(traceIsValid);
  callretIsValid=verifyCallRet(it->getTraceInstForReadOnly());
  EXPECT_TRUE(callretIsValid);
  //child 2 of child process
  it->updateInstTrace(traceFileNameChild2OfChild);
  EXPECT_TRUE(it->isForkQueueEmpty());
  callretIsValid=verifyCallRet(it->getTraceInstForReadOnly());
  //child process 2
  it->updateInstTrace(traceFileNameChild2);
  EXPECT_FALSE(it->isForkQueueEmpty());
  //traceIsValid=verifyInstTrace(it->getTraceInstForReadOnly());
  //EXPECT_TRUE(traceIsValid);
  callretIsValid=verifyCallRet(it->getTraceInstForReadOnly());
  EXPECT_TRUE(callretIsValid);
  //child of child process
  char const* traceFileNameChildOfChild2=it->getLogFileName();
  it->popForkQueue();
  char const* traceFileNameChild2OfChild2=it->getLogFileName();
  it->updateInstTrace(traceFileNameChildOfChild);
  EXPECT_TRUE(it->isForkQueueEmpty());
  //traceIsValid=verifyInstTrace(it->getTraceInstForReadOnly());
  //EXPECT_TRUE(traceIsValid);
  callretIsValid=verifyCallRet(it->getTraceInstForReadOnly());
  EXPECT_TRUE(callretIsValid);
  it->updateInstTrace(traceFileNameChild2OfChild2);
  EXPECT_TRUE(it->isForkQueueEmpty());
  callretIsValid=verifyCallRet(it->getTraceInstForReadOnly());
  EXPECT_TRUE(callretIsValid);
}
/*
TEST_F(InstTraceFix, instTrace_vsftpd){
	string logdir("/home/jun/straight-DTA/vsftpd-2.2.2/test");
	string cfigfile("configFile");

	debug_utils.reset(new DEBUG_utils());
	sigHandlerSet.reset(new INPUT_SigHandlerSet( (logdir+"/"+"sigHandlerFile").c_str() ));
	globalData.reset(new INPUT_GlobalData(*m2));

	INPUT_InstTrace* it=new INPUT_InstTrace( *m2, logdir, cfigfile );
	auto instTrace1=it->getTraceInstForReadOnly();
	EXPECT_TRUE(instTrace1.size()>0);
	bool traceIsValid=verifyInstTrace(instTrace1);
	EXPECT_TRUE(traceIsValid);
	if(traceIsValid==false){
		//dumpInstTrace(instTrace1);
	}
	EXPECT_FALSE(it->isForkQueueEmpty());
	//child process
	char const* traceFileNameChild=it->getLogFileName();
	it->updateInstTrace(traceFileNameChild);
	EXPECT_FALSE(it->isForkQueueEmpty());
	auto instTrace2=it->getTraceInstForReadOnly();
	EXPECT_TRUE(instTrace2.size()>0);
	traceIsValid=verifyInstTrace(instTrace2);
	EXPECT_TRUE(traceIsValid);
	if(traceIsValid==false){
		//dumpInstTrace(instTrace2);
	}
	//child1 of child process
	EXPECT_FALSE(it->isForkQueueEmpty());
	char const * traceFileNameChild1OfChild=it->getLogFileName();
	it->updateInstTrace(traceFileNameChild1OfChild);
	EXPECT_TRUE(it->isForkQueueEmpty());
	auto instTrace3=it->getTraceInstForReadOnly();
	EXPECT_TRUE(instTrace3.size()>0);
	traceIsValid=verifyInstTrace(instTrace3);
	EXPECT_TRUE(traceIsValid);
}
*/

/*
TEST_F(InstTraceFix, instTrace_secondChildOfChild_vsftpd){
  string logdir("/home/jun/straight-DTA/vsftpd-2.2.2/test");
  string cfigfile("configFile");

  debug_utils.reset(new DEBUG_utils());
  sigHandlerSet.reset(new INPUT_SigHandlerSet( (logdir+"/"+"sigHandlerFile").c_str() ));
  globalData.reset(new INPUT_GlobalData(*m2));

  INPUT_InstTrace* it=new INPUT_InstTrace( *m2, logdir, cfigfile );
  char const* traceFileNameChild2OfChild="tmp.27344.full";
  it->updateInstTrace(traceFileNameChild2OfChild);
  auto forkQueueIsEmpty=it->isForkQueueEmpty();
  if(forkQueueIsEmpty==false){
    cout<<"ForkQueue is not empty and the ront is : "<<it->getForkQueueFront()<<"\n";
  }
  EXPECT_TRUE(forkQueueIsEmpty);
  auto instTrace3=it->getTraceInstForReadOnly();
  EXPECT_TRUE(instTrace3.size()>0);
  auto traceIsValid=verifyInstTrace(instTrace3);
  EXPECT_TRUE(traceIsValid);
}
*/

/*
TEST_F(InstTraceFix, iterator_vftpd){
	string logdir("/home/jun/straight-DTA/vsftpd-2.2.2/test");
	string cfigfile("configFile");

	debug_utils.reset(new DEBUG_utils());
	sigHandlerSet.reset(new INPUT_SigHandlerSet( (logdir+"/"+"sigHandlerFile").c_str() ));
	globalData.reset(new INPUT_GlobalData(*m2));

	INPUT_InstTrace* it=new INPUT_InstTrace( *m2, logdir, cfigfile );

	auto instTrace=it->getTraceInstForReadOnly();
	auto begin=it->getTraceInstBegin();
	auto end=it->getTraceInstEnd();
	long numInst1=0;
	for(auto i=begin; i!=end; i++){
		numInst1++;
	}
	EXPECT_TRUE(numInst1>0);
	for(auto i=instTrace.begin(), i_e=instTrace.end(); i!=i_e; i++){
		numInst1--;
	}
	EXPECT_TRUE(numInst1==0);
	bool traceAreSame=true;
	if(numInst1==0){
		for(auto i=instTrace.begin(), i_e=instTrace.end(); i!=i_e; i++, begin++){
			Instruction const* k1=*i;
			Instruction const* k2=*begin;
			if(k1->getOpcode()!=k2->getOpcode()){
				traceAreSame=false;
				break;
			}
		}
		EXPECT_TRUE(traceAreSame);
	}
}
*/
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
