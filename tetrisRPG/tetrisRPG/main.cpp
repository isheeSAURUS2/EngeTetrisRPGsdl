#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

using namespace std;

int number;
int randnumber;
int main(int argc, char* argv[]) {
	randnumber = rand() % 1000;
	cout << "hello please enter your favorite number here: \n";
	cin >> number;
	cout << "ahhh i see your favorite number is : " << number << endl;
	cout << "my favorite number is : " << randnumber << endl;
	if (number == randnumber) {
		cout << "wow we have the same favorite number C:" << endl;
	} else {
		cout << "oh well, we have different favorite numbers but yours is asssssssss." << endl;
	}
		return 0;
}
