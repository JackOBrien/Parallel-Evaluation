#include <stdio.h>
#include <iostream>
#include <string.h>
#include <queue>
#include <stack>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#define READ 0
#define WRITE 1
#define MAX 1024

using namespace std;

struct node {
	string data;
	node *left;
	node *right;
	node *parent;
};

vector<string> infixToPrefix(string infix);
bool isFloat(string c);
int priority(string c);
bool isOperator(string c);
node* buildTree(const vector<string> &prefix);
void buildTree(node *n, const vector<string> &prefix, int index);
float evaluateTree(node *n);

void sigInt (int);
void sigUsr (int sigNum);

bool imm;

float evaluateTree(node *n) {

    float leftSide, rightSide;
	
	pid_t left_pid, right_pid;
	
	int leftPipe[2];
	int rightPipe[2];
	
	bool wentLeft, wentRight;
	wentLeft = wentRight = false;
	
	/* Left Side */
    if (isOperator(n->left->data)) {
	
		wentLeft = true;
	
        if (pipe(leftPipe) < 0) {
			perror ("Unable to pipe.");
			exit (1);
		}
		
		if ((left_pid = fork()) < 0) {
			perror ("Fork failed.");
			exit (1);
		}
		
		/* Child */
		if (!left_pid) {
			close (leftPipe[READ]);
			
			cout << "Parent " << getppid() << " created child process: " << getpid() <<endl;
			
			//sleep(8);
			float val = evaluateTree(n->left);
			
			write (leftPipe[WRITE], &val, sizeof(float));
			exit (0);
		}
    } else {
		leftSide = stof(n->left->data);
	}

	/* Right Side */
    if (isOperator(n->right->data)) {
	
		wentRight = true;
	
        if (pipe(rightPipe) < 0) {
			perror ("Unable to pipe.");
			exit (1);
		}
		
		if ((right_pid = fork ()) < 0) {
			perror ("Fork failed.");
			exit (1);
		}
		
		/* Child */
		if (!right_pid) {
			close (rightPipe[READ]);
			
			cout << "Parent " << getppid() << " created child process: " << getpid() <<endl;
			
			//sleep (8);
			float val = evaluateTree(n->right);
			
			write (rightPipe[WRITE], &val, sizeof(float));
			exit (0);
		}
    } else {
		rightSide = stof(n->right->data);
	}
	
	if (wentLeft) {
		int status;
		
		waitpid(left_pid, &status, WNOHANG);
		
		close(leftPipe[WRITE]);
		read(leftPipe[READ], &leftSide, sizeof(float));
	}

	if (wentRight) {
		int status;
		
		waitpid(right_pid, &status, WNOHANG);
	
		close(rightPipe[WRITE]);
		read(rightPipe[READ], &rightSide, sizeof(float));
	}

	while (!imm);
	
	string c = n->data;
	
	cout << "Process " << getpid() << " :: ";
	cout << leftSide << " " << c << " " << rightSide << endl;
	
	if (c.compare("*") == 0) return leftSide * rightSide;
	if (c.compare("/") == 0) return leftSide / rightSide;
	if (c.compare("+") == 0) return leftSide + rightSide;
	if (c.compare("-") == 0) return leftSide - rightSide;
}


node* buildTree(const vector<string> &prefix) {
	node *root = new node;
	root->data = prefix.at(0);
	root->left = NULL;
	root->right = NULL;
	root->parent = NULL;
	
	buildTree(root, prefix, 1);
	
	return root;
}

void buildTree(node *n, const vector<string> &prefix, int index) {
	
	/* End condition */
	if (n == NULL || index == prefix.size()) {
		return;
	}
	
	/* Current node is operator */
	if (isOperator(n->data)) {
		
		if (n->left == NULL) {
			node *left = new node;
			left->data = prefix.at(index);
			left->left = NULL;
			left->right = NULL;
			left->parent = n;
			
			n->left = left;
			
			buildTree(left, prefix, ++index);
		} 
		
		else if (n->right == NULL) {
			node *right = new node;
			right->data = prefix.at(index);
			right->left = NULL;
			right->right = NULL;
			right->parent = n;
			
			n->right = right;
			
			buildTree(right, prefix, ++index);
		}
		
		else {
			buildTree(n->parent, prefix, index);
		}
	}
	
	/* Current node is number */
	else {
		buildTree(n->parent, prefix, index);
	}
}

bool isOperator(string c) {
	return priority(c) > 0;
}

int priority(string c) {
		
	if (c.compare("*") == 0) return 4;
	if (c.compare("/") == 0) return 4;
	if (c.compare("+") == 0) return 2;
	if (c.compare("-") == 0) return 2;
	return 0;
}

bool isFloat(string c) {
	std::istringstream iss(c);
	float f;
	iss >> noskipws >> f; 
	return iss.eof() && !iss.fail(); 
}

vector<string> infixToPrefix(string infix) {	
    stack<string> stackk;
    stack<string> strng;
	
	vector<string> tokens;
		
	istringstream iss(infix);
		
	copy(istream_iterator<string>(iss),
		istream_iterator<string>(),
		back_inserter(tokens));
			
	for (int i = tokens.size() - 1; i >= 0; i--){

        if(isFloat(tokens.at(i))) {
            strng.push(tokens.at(i));
        } else {
            if(stackk.empty()||(priority(stackk.top()) < priority(tokens.at(i)))) {
                stackk.push(tokens.at(i));
            } else {
                while(!stackk.empty()&&priority(stackk.top())>priority(tokens.at(i))) {
                    strng.push(stackk.top());
                    stackk.pop();
                }
                stackk.push(tokens.at(i));
            }
        }
    }

    while(!stackk.empty()) {
        strng.push(stackk.top());
        stackk.pop();
    }
	
	vector<string> prefix;
	
	while(!strng.empty()) {
            prefix.push_back(strng.top());
            strng.pop();
    }
	
	return prefix;
}

void sigInt (int sigNum) {
	imm = true;
}

void sigUsr (int sigNum) {
	
}

float evaluate (const char* infix_expr, bool immediate) {
	
	signal (SIGINT, sigInt);
	
	string infix(infix_expr);
	
	char infix_copy[infix.length()];
	size_t length = infix.copy(infix_copy, infix.length(), 0);
	infix_copy[length] = '\0';
	
	vector<string> prefix = infixToPrefix(infix_copy);
	
	node* root = buildTree(prefix);

	imm = immediate;
	
	cout << "\nParent process: " << getpid() <<endl;
	
	return evaluateTree(root);
}

int main() {
	
	printf ("Test 1 %f\n", evaluate ("2.0 * 3.0", false));
	printf ("Test 2 %f\n", evaluate ("200.0 + 300.0", false));
	printf ("Test 3 %f\n", evaluate ("10.0 / 5.0", false));
	printf ("Test 4 %f\n", evaluate ("16.0 - 10.5", false));
	printf ("Test 5 %f\n", evaluate ("2 + 3 * 4", false));
	printf ("Test 6 %f\n", evaluate ("2 * 3 + 4", false));
	printf ("Test 7 %f\n", evaluate ("2 * 3 + 4 / 5", false)); 
	printf ("Test 8 %f\n", evaluate ("2 + 3 * 4 - 5 / 6", false));
}