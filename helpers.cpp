#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include "helpers.hpp"

using namespace std;

void Helpers::printQueue(queue<Instruction> q)
{
	while (!q.empty()){
		cout<<q.front()<<endl;
		q.pop();
	}
	cout<<endl;
}
