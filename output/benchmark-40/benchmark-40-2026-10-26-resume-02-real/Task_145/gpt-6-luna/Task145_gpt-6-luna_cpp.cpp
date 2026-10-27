#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

long long maxSubarraySum(const vector<long long>& values) {
    long long current = values[0];
    long long best = values[0];

    for (size_t i = 1; i < values.size(); ++i) {
        current = max(values[i], current + values[i]);
        best = max(best, current);
    }

    return best;
}

int main() {
    int n;
    if (!(cin >> n) || n <= 0) {
        return 0;
    }

    vector<long long> values(n);
    for (long long& value : values) {
        cin >> value;
    }

    cout << maxSubarraySum(values) << '\n';
    return 0;
}