TARGET=toy-cp
ORIGIN=toy-cp.orig
DIR=..
SRC=$(DIR)/$(TARGET)
NEWSRC=$(DIR)/new$(TARGET)
INIT=../../straightDFA/init
LOG=../../straightDFA/log

all:$(TARGET) $(ORIGIN)

$(TARGET):$(TARGET).s
	gcc -g -pthread $^ -o $@

$(TARGET).s:$(TARGET).bc
	strace -f -o opt.inline.strace opt -load /home/jun/llvm-3.x/build/Release+Debug+Asserts/lib/LLVMHello.so -inline-log -inline< $^ > inline.bc
	llc inline.bc -o $@

$(TARGET).bc:$(NEWSRC).bc $(INIT).bc $(LOG).bc
	llvm-link $^ -o $@

$(INIT).bc:$(INIT).c $(LOG).h
	clang -g -c -emit-llvm $< -o $@

$(LOG).bc:$(LOG).c $(LOG).h
	clang -g -c -emit-llvm $< -o $@

$(NEWSRC).bc:$(SRC).bc
	opt -load /home/jun/llvm-3.x/build/Release+Debug+Asserts/lib/LLVMHello.so -inst < $^ > $@

$(ORIGIN):$(SRC).bc
	llc $^ -o orig.s
	gcc -g -pthread orig.s -o $@

$(SRC).bc:$(SRC).c
	clang -g -c -emit-llvm $^ -o $@

clean:$(SRC).c $(INIT).c $(LOG).c
	rm $(TARGET) $(TARGET).s $(TARGET).bc
	touch $^

run:$(TARGET)
	./$(TARGET) src.txt dst.txt	

view:$(TARGET).bc
	llvm-dis $^ -o $(TARGET).ll

reco:$(TARGET).bc
	opt -load /home/jun/llvm-3.x/build/Release+Debug+Asserts/lib/LLVMHello.so -reco < $^
