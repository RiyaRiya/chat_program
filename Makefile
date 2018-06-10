all: a3chat

clean:
	rm -rf *o a3chat
a3chat: a3chat.cpp
	gcc -o a3chat a3chat.cpp
tar: Makefile a3chat.cpp a3chat Project_Report.pdf
	tar cvf submit.tar Makefile a3chat a3chat.cpp Project_Report.pdf 


