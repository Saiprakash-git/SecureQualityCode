Reading the task: write a C++ program with a function findNb taking m (long long), returning n or -1. Include a main that reads input and prints result to be a complete file. Keep it simple and correct.

Sum of cubes 1..n = (n(n+1)/2)^2. Loop n from 1 while sum < m, increment; if sum == m return n; else -1.

#include <iostream>
using namespace std;

long long findNb(long long m) {
    if (m <= 0) return -1;
    long long sum = 0;
    long long n = 0;
    while (sum < m) {
        n++;
        sum += n * n * n;
    }
    return (sum == m) ? n : -1;
}

int main() {
    long long m;
    if (cin >> m) {
        cout << findNb(m) << endl;
    }
    return 0;
}