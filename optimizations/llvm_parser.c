#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

#define prt(x) if(x) { printf("%s\n", x); }

// Data structure for storing sets of store instructions
typedef struct StoreSet {
	LLVMValueRef *stores;
	int count;
	int capacity;
} StoreSet;

// Data structure for each basic block
typedef struct BBData {
	LLVMBasicBlockRef bb;
	StoreSet gen;
	StoreSet kill;
	StoreSet in;
	StoreSet out;
	LLVMBasicBlockRef *predecessors;
	int num_predecessors;
} BBData;


/* This function reads the given llvm file and loads the LLVM IR into
	 data-structures that we can works on for optimization phase.
*/
LLVMModuleRef createLLVMModel(char * filename){
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	if (err != NULL) { 
		prt(err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		prt(err);
	}

	return m;
}

bool instructionsMatch(LLVMValueRef instA, LLVMValueRef instB) {
	LLVMOpcode opA = LLVMGetInstructionOpcode(instA);
	LLVMOpcode opB = LLVMGetInstructionOpcode(instB);
	
	if (opA != opB) {
		return false;
	}
	
	int numOpsA = LLVMGetNumOperands(instA);
	int numOpsB = LLVMGetNumOperands(instB);
	
	if (numOpsA != numOpsB) {
		return false;
	}
	
	for (int i = 0; i < numOpsA; i++) {
		LLVMValueRef opA = LLVMGetOperand(instA, i);
		LLVMValueRef opB = LLVMGetOperand(instB, i);
		
		if (opA != opB) {
			return false;
		}
	}
	
	return true;
}


bool hasInterferingStore(LLVMBasicBlockRef bb, LLVMValueRef loadA, LLVMValueRef loadB) {
	LLVMValueRef ptrA = LLVMGetOperand(loadA, 0);
	LLVMValueRef ptrB = LLVMGetOperand(loadB, 0);
	
	if (ptrA != ptrB) {
		return false;
	}
	
	bool foundA = false;
	
	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
			inst = LLVMGetNextInstruction(inst)) {
		
		if (inst == loadA) {
			foundA = true;
			continue;
		}
		
		if (!foundA) {
			continue;
		}
		
		if (inst == loadB) {
			break;
		}
		
		LLVMOpcode op = LLVMGetInstructionOpcode(inst);
		if (op == LLVMStore) {
			LLVMValueRef storePtr = LLVMGetOperand(inst, 1);
			
			if (storePtr == ptrA) {
				return true;
			}
		}
	}
	
	return false;
}

void commonSubexpressionElimination(LLVMBasicBlockRef bb) {
	for (LLVMValueRef instA = LLVMGetFirstInstruction(bb); instA;
			instA = LLVMGetNextInstruction(instA)) {
		
		if (LLVMIsATerminatorInst(instA)) {
			continue;
		}
		
		LLVMOpcode opA = LLVMGetInstructionOpcode(instA);
		
		for (LLVMValueRef instB = LLVMGetNextInstruction(instA); instB;
				instB = LLVMGetNextInstruction(instB)) {
			
			if (LLVMIsATerminatorInst(instB)) {
				continue;
			}
			
			if (instructionsMatch(instA, instB)) {
				// check for interfering stores
				if (opA == LLVMLoad) {
					if (!hasInterferingStore(bb, instA, instB)) {
						LLVMReplaceAllUsesWith(instB, instA);
					}
				} else {
					LLVMReplaceAllUsesWith(instB, instA);
				}
			}
		}
	}
}


bool hasSideEffects(LLVMValueRef inst) {
	LLVMOpcode op = LLVMGetInstructionOpcode(inst);
	
	if (op == LLVMStore) {
		return true;
	}
	
	if (op == LLVMCall || op == LLVMInvoke) {
		return true;
	}

	if (LLVMIsATerminatorInst(inst)) {
		return true;
	}
	
	return false;
}


void deadCodeElimination(LLVMValueRef function) {
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
			bb = LLVMGetNextBasicBlock(bb)) {
		
		LLVMValueRef inst = LLVMGetFirstInstruction(bb);
		
		while (inst) {
			LLVMValueRef nextInst = LLVMGetNextInstruction(inst);
			
			LLVMUseRef firstUse = LLVMGetFirstUse(inst);
			
			if (firstUse == NULL && !hasSideEffects(inst)) {
				LLVMInstructionEraseFromParent(inst);
			}
			
			inst = nextInst;
		}
	}
}

void constantFolding(LLVMValueRef function) {
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
			bb = LLVMGetNextBasicBlock(bb)) {
		
		for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
				inst = LLVMGetNextInstruction(inst)) {
			
			if (LLVMIsATerminatorInst(inst)) {
				continue;
			}
			
			LLVMOpcode op = LLVMGetInstructionOpcode(inst);
			
			// arithmetic operation
			if (op == LLVMAdd || op == LLVMSub || op == LLVMMul) {
				int numOps = LLVMGetNumOperands(inst);
				if (numOps == 2) {
					LLVMValueRef op0 = LLVMGetOperand(inst, 0);
					LLVMValueRef op1 = LLVMGetOperand(inst, 1);
					
					if (LLVMIsConstant(op0) && LLVMIsConstant(op1)) {
						LLVMValueRef constResult = NULL;
						
						if (op == LLVMAdd) {
							constResult = LLVMConstAdd(op0, op1);
						} else if (op == LLVMSub) {
							constResult = LLVMConstSub(op0, op1);
						} else if (op == LLVMMul) {
							constResult = LLVMConstMul(op0, op1);
						}
						
						if (constResult != NULL) {
							LLVMReplaceAllUsesWith(inst, constResult);
						}
					}
				}
			}
		}
	}
}

// Helper functions for StoreSet management
void initStoreSet(StoreSet *set) {
	set->capacity = 16;
	set->count = 0;
	set->stores = (LLVMValueRef *)malloc(set->capacity * sizeof(LLVMValueRef));
}

void freeStoreSet(StoreSet *set) {
	if (set->stores) {
		free(set->stores);
		set->stores = NULL;
	}
	set->count = 0;
	set->capacity = 0;
}

void addToStoreSet(StoreSet *set, LLVMValueRef store) {
	// check if already in set
	for (int i = 0; i < set->count; i++) {
		if (set->stores[i] == store) {
			return;
		}
	}
	
	// resize if needed
	if (set->count >= set->capacity) {
		set->capacity *= 2;
		set->stores = (LLVMValueRef *)realloc(set->stores, set->capacity * sizeof(LLVMValueRef));
	}
	
	set->stores[set->count++] = store;
}

void removeFromStoreSet(StoreSet *set, LLVMValueRef store) {
	for (int i = 0; i < set->count; i++) {
		if (set->stores[i] == store) {
			// move last element to this position
			set->stores[i] = set->stores[set->count - 1];
			set->count--;
			return;
		}
	}
}

bool containsStore(StoreSet *set, LLVMValueRef store) {
	for (int i = 0; i < set->count; i++) {
		if (set->stores[i] == store) {
			return true;
		}
	}
	return false;
}

void unionStoreSet(StoreSet *dest, StoreSet *src) {
	for (int i = 0; i < src->count; i++) {
		addToStoreSet(dest, src->stores[i]);
	}
}

void subtractStoreSet(StoreSet *dest, StoreSet *src) {
	for (int i = 0; i < src->count; i++) {
		removeFromStoreSet(dest, src->stores[i]);
	}
}

bool storeSetsEqual(StoreSet *a, StoreSet *b) {
	if (a->count != b->count) {
		return false;
	}
	for (int i = 0; i < a->count; i++) {
		if (!containsStore(b, a->stores[i])) {
			return false;
		}
	}
	return true;
}

void copyStoreSet(StoreSet *dest, StoreSet *src) {
	dest->count = 0;
	for (int i = 0; i < src->count; i++) {
		addToStoreSet(dest, src->stores[i]);
	}
}

bool storesWriteToSameLocation(LLVMValueRef store1, LLVMValueRef store2) {
	if (store1 == store2) {
		return true;
	}
	
	LLVMValueRef ptr1 = LLVMGetOperand(store1, 1);
	LLVMValueRef ptr2 = LLVMGetOperand(store2, 1);
	
	return (ptr1 == ptr2);
}

bool isConstantStore(LLVMValueRef store) {
	LLVMValueRef value = LLVMGetOperand(store, 0);
	return LLVMIsConstant(value);
}

LLVMValueRef getConstantFromStore(LLVMValueRef store) {
	return LLVMGetOperand(store, 0);
}

bool constantsEqual(LLVMValueRef const1, LLVMValueRef const2) {
	return (const1 == const2);
}

void buildPredecessors(LLVMValueRef function, BBData *bbData, int numBlocks) {
	for (int i = 0; i < numBlocks; i++) {
		bbData[i].num_predecessors = 0;
		bbData[i].predecessors = NULL;
	}
	
	// count predecessors for each block
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
			bb = LLVMGetNextBasicBlock(bb)) {
		
		LLVMValueRef terminator = LLVMGetBasicBlockTerminator(bb);
		if (terminator == NULL) {
			continue;
		}
		
		unsigned numSuccessors = LLVMGetNumSuccessors(terminator);
		for (unsigned i = 0; i < numSuccessors; i++) {
			LLVMBasicBlockRef succ = LLVMGetSuccessor(terminator, i);
			
			// find the index of the successor block
			for (int j = 0; j < numBlocks; j++) {
				if (bbData[j].bb == succ) {
					bbData[j].num_predecessors++;
					break;
				}
			}
		}
	}

	for (int i = 0; i < numBlocks; i++) {
		if (bbData[i].num_predecessors > 0) {
			bbData[i].predecessors = (LLVMBasicBlockRef *)malloc(
				bbData[i].num_predecessors * sizeof(LLVMBasicBlockRef));
			bbData[i].num_predecessors = 0; // reset to use as index
		}
	}
	
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
			bb = LLVMGetNextBasicBlock(bb)) {
		
		LLVMValueRef terminator = LLVMGetBasicBlockTerminator(bb);
		if (terminator == NULL) {
			continue;
		}
		
		unsigned numSuccessors = LLVMGetNumSuccessors(terminator);
		for (unsigned i = 0; i < numSuccessors; i++) {
			LLVMBasicBlockRef succ = LLVMGetSuccessor(terminator, i);
			
			// find the index of the successor block and add predecessor
			for (int j = 0; j < numBlocks; j++) {
				if (bbData[j].bb == succ) {
					int idx = bbData[j].num_predecessors;
					bbData[j].predecessors[idx] = bb;
					bbData[j].num_predecessors++;
					break;
				}
			}
		}
	}
}

void computeGEN(LLVMBasicBlockRef bb, StoreSet *gen, StoreSet *allStores) {
	initStoreSet(gen);
	
	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
			inst = LLVMGetNextInstruction(inst)) {
		
		LLVMOpcode op = LLVMGetInstructionOpcode(inst);
		
		if (op == LLVMStore) {
			addToStoreSet(gen, inst);
			
			// remove stores that write to the same memory location
			for (int i = gen->count - 1; i >= 0; i--) {
				LLVMValueRef existingStore = gen->stores[i];
				if (existingStore != inst && storesWriteToSameLocation(inst, existingStore)) {
					removeFromStoreSet(gen, existingStore);
				}
			}
		}
	}
}


void computeKILL(LLVMBasicBlockRef bb, StoreSet *kill, StoreSet *allStores) {
	initStoreSet(kill);
	
	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
			inst = LLVMGetNextInstruction(inst)) {
		
		LLVMOpcode op = LLVMGetInstructionOpcode(inst);
		
		if (op == LLVMStore) {
			for (int i = 0; i < allStores->count; i++) {
				LLVMValueRef otherStore = allStores->stores[i];
				if (otherStore != inst && storesWriteToSameLocation(inst, otherStore)) {
					addToStoreSet(kill, otherStore);
				}
			}
		}
	}
}

// return true if any changes were made
bool globalConstantPropagation(LLVMValueRef function) {
	int numBlocks = 0;
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
			bb = LLVMGetNextBasicBlock(bb)) {
		numBlocks++;
	}
	
	if (numBlocks == 0) {
		return false;
	}
	
	BBData *bbData = (BBData *)calloc(numBlocks, sizeof(BBData));
	
	int idx = 0;
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
			bb = LLVMGetNextBasicBlock(bb)) {
		bbData[idx].bb = bb;
		initStoreSet(&bbData[idx].gen);
		initStoreSet(&bbData[idx].kill);
		initStoreSet(&bbData[idx].in);
		initStoreSet(&bbData[idx].out);
		bbData[idx].predecessors = NULL;
		bbData[idx].num_predecessors = 0;
		idx++;
	}
	
	// collect all store instructions in the function
	StoreSet allStores;
	initStoreSet(&allStores);
	for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb;
			bb = LLVMGetNextBasicBlock(bb)) {
		for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst;
				inst = LLVMGetNextInstruction(inst)) {
			if (LLVMGetInstructionOpcode(inst) == LLVMStore) {
				addToStoreSet(&allStores, inst);
			}
		}
	}
	
	for (int i = 0; i < numBlocks; i++) {
		computeGEN(bbData[i].bb, &bbData[i].gen, &allStores);
		computeKILL(bbData[i].bb, &bbData[i].kill, &allStores);
	}
	
	// build predecessor information
	buildPredecessors(function, bbData, numBlocks);
	
	// initialize OUT sets to GEN sets
	for (int i = 0; i < numBlocks; i++) {
		copyStoreSet(&bbData[i].out, &bbData[i].gen);
	}
	
	bool change = true;
	while (change) {
		change = false;
		
		for (int i = 0; i < numBlocks; i++) {
			// IN[B] = union of OUT[P] for all predecessors P
			StoreSet oldIn;
			initStoreSet(&oldIn);
			copyStoreSet(&oldIn, &bbData[i].in);
			
			bbData[i].in.count = 0;
			
			for (int j = 0; j < bbData[i].num_predecessors; j++) {
				LLVMBasicBlockRef pred = bbData[i].predecessors[j];
				
				for (int k = 0; k < numBlocks; k++) {
					if (bbData[k].bb == pred) {
						unionStoreSet(&bbData[i].in, &bbData[k].out);
						break;
					}
				}
			}
			
			// OUT[B] = GEN[B] union (IN[B] - KILL[B])
			StoreSet oldOut;
			initStoreSet(&oldOut);
			copyStoreSet(&oldOut, &bbData[i].out);
			
			copyStoreSet(&bbData[i].out, &bbData[i].in);
			subtractStoreSet(&bbData[i].out, &bbData[i].kill);
			
			unionStoreSet(&bbData[i].out, &bbData[i].gen);
			
			if (!storeSetsEqual(&bbData[i].out, &oldOut)) {
				change = true;
			}
			
			freeStoreSet(&oldIn);
			freeStoreSet(&oldOut);
		}
	}
	
	// replace loads with constants based on reaching stores
	bool anyChange = false;
	
	for (int i = 0; i < numBlocks; i++) {
		StoreSet R;
		initStoreSet(&R);
		copyStoreSet(&R, &bbData[i].in);
		
		// collect load instructions to delete
		LLVMValueRef *loadsToDelete = NULL;
		int numLoadsToDelete = 0;
		int capacityLoadsToDelete = 16;
		loadsToDelete = (LLVMValueRef *)malloc(capacityLoadsToDelete * sizeof(LLVMValueRef));
		
		for (LLVMValueRef inst = LLVMGetFirstInstruction(bbData[i].bb); inst;
				inst = LLVMGetNextInstruction(inst)) {
			
			LLVMOpcode op = LLVMGetInstructionOpcode(inst);
			
			if (op == LLVMStore) {
				addToStoreSet(&R, inst);
				
				for (int j = R.count - 1; j >= 0; j--) {
					LLVMValueRef existingStore = R.stores[j];
					if (existingStore != inst && storesWriteToSameLocation(inst, existingStore)) {
						removeFromStoreSet(&R, existingStore);
					}
				}
			} else if (op == LLVMLoad) {
				LLVMValueRef loadPtr = LLVMGetOperand(inst, 0);
				
				StoreSet reachingStores;
				initStoreSet(&reachingStores);
				
				for (int j = 0; j < R.count; j++) {
					LLVMValueRef store = R.stores[j];
					LLVMValueRef storePtr = LLVMGetOperand(store, 1);
					if (storePtr == loadPtr) {
						addToStoreSet(&reachingStores, store);
					}
				}
				
				if (reachingStores.count > 0) {
					bool allConstant = true;
					LLVMValueRef constantValue = NULL;
					
					for (int j = 0; j < reachingStores.count; j++) {
						LLVMValueRef store = reachingStores.stores[j];
						if (!isConstantStore(store)) {
							allConstant = false;
							break;
						}
						
						LLVMValueRef storeConst = getConstantFromStore(store);
						if (constantValue == NULL) {
							constantValue = storeConst;
						} else {
							// Check if this constant equals the first one
							if (!constantsEqual(constantValue, storeConst)) {
								allConstant = false;
								break;
							}
						}
					}
					
					if (allConstant && constantValue != NULL) {
						LLVMReplaceAllUsesWith(inst, constantValue);
						
						if (numLoadsToDelete >= capacityLoadsToDelete) {
							capacityLoadsToDelete *= 2;
							loadsToDelete = (LLVMValueRef *)realloc(loadsToDelete, 
								capacityLoadsToDelete * sizeof(LLVMValueRef));
						}
						loadsToDelete[numLoadsToDelete++] = inst;
						anyChange = true;
					}
				}
				
				freeStoreSet(&reachingStores);
			}
		}
		
		// delete marked load instructions
		for (int j = 0; j < numLoadsToDelete; j++) {
			LLVMInstructionEraseFromParent(loadsToDelete[j]);
		}
		free(loadsToDelete);
		
		freeStoreSet(&R);
	}
	
	freeStoreSet(&allStores);
	for (int i = 0; i < numBlocks; i++) {
		freeStoreSet(&bbData[i].gen);
		freeStoreSet(&bbData[i].kill);
		freeStoreSet(&bbData[i].in);
		freeStoreSet(&bbData[i].out);
		if (bbData[i].predecessors) {
			free(bbData[i].predecessors);
		}
	}
	free(bbData);
	
	return anyChange;
}


void walkBBInstructions(LLVMBasicBlockRef bb){
	for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction;
  				instruction = LLVMGetNextInstruction(instruction)) {
	
		LLVMOpcode op = LLVMGetInstructionOpcode(instruction);
    
		if (LLVMIsABranchInst(instruction)) {
			printf("Inst: \n");
			LLVMDumpValue(instruction);
			printf("     %d\n", LLVMGetNumOperands(instruction));
			
			if (LLVMIsConditional(instruction)){
				LLVMValueRef b1 = LLVMGetOperand(instruction, 1); 
				LLVMValueRef b2 = LLVMGetOperand(instruction, 2);
				printf("\n");
				LLVMDumpValue(b1);
				printf("\n");
				LLVMDumpValue(b2);
				printf("\n");
			} 
      	}
		
		if (op == LLVMAdd) {
			printf("Found add instruction\n");
			LLVMDumpValue(instruction);		
			printf("\n");
			//Checking if one of the operands in a constant
			if (LLVMIsConstant(LLVMGetOperand(instruction, 0)) ||  
          LLVMIsConstant(LLVMGetOperand(instruction, 1))) {
			// Walking over all uses of an instruction
				LLVMUseRef y = LLVMGetFirstUse(instruction);
				if (y == NULL) printf("No uses\n");
				else printf("\n Exploring users of add with constant operand:\n");
		  	for (LLVMUseRef u = LLVMGetFirstUse(instruction);
					u;
					u = LLVMGetNextUse(u)){
					LLVMValueRef x = LLVMGetUser(u);
					LLVMDumpValue(x);
					printf("\n");

				}
			}
		}
 	}
}


void walkBasicblocks(LLVMValueRef function){
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			 basicBlock;
  			 basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
		printf("In basic block\n");
		
		commonSubexpressionElimination(basicBlock);
		
		walkBBInstructions(basicBlock);
		LLVMValueRef terminator = LLVMGetBasicBlockTerminator(basicBlock);
		printf("\nTerminator: ");
		LLVMDumpValue(terminator);		

	}
}

void walkFunctions(LLVMModuleRef module){
	for (LLVMValueRef function =  LLVMGetFirstFunction(module); 
			function; 
			function = LLVMGetNextFunction(function)) {

		const char* funcName = LLVMGetValueName(function);	

		printf("Function Name: %s\n", funcName);

		// Perform constant propagation and constant folding in a loop until fixpoint
		bool changed = true;
		int iterations = 0;
		const int maxIterations = 10; // Safety limit
		
		while (changed && iterations < maxIterations) {
			changed = false;
			iterations++;
			
			if (globalConstantPropagation(function)) {
				changed = true;
			}
			
			constantFolding(function);
			
			deadCodeElimination(function);
		}
		
		walkBasicblocks(function);
 	}
}

void walkGlobalValues(LLVMModuleRef module){
	for (LLVMValueRef gVal =  LLVMGetFirstGlobal(module);
                        gVal;
                        gVal = LLVMGetNextGlobal(gVal)) {

                const char* gName = LLVMGetValueName(gVal);
                printf("Global variable name: %s\n", gName);
        }
}

int main(int argc, char** argv)
{
	LLVMModuleRef m;

	if (argc == 2){
		m = createLLVMModel(argv[1]);
	}
	else{
		m = NULL;
		return 1;
	}

	if (m != NULL){
		//LLVMDumpModule(m);
		walkGlobalValues(m);
		walkFunctions(m);
		LLVMPrintModuleToFile (m, "test_new.ll", NULL);
	}
	else {
	    fprintf(stderr, "m is NULL\n");
	}
	
	return 0;
}
