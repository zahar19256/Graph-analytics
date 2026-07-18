#include <bits/stdc++.h>
#include <thread>

using namespace std;

const int N = 2e5 + 100;

vector <int> g[N];
double value[N][2];

int main() {
    int n , m;
    cin >> n >> m;
    for (int i = 0; i < m; ++i) {
        int x , y;
        cin >> x >> y;
        --x;
        --y;
        g[x].push_back(y);
    }
    for (int i = 0; i < n; ++i) {
        value[i][0] = 1;
        g[n].push_back(i);
        g[i].push_back(n);
    }
    int step = 0;
    double treshhold = 1e-3;
    while (true) {
        for (int i = 0; i <= n; ++i) {
            value[i][1 - step] = 0;
        }
        for (int i = 0; i <= n; ++i) {
            for (auto to : g[i]) {
                value[to][1 - step] += (1.0 / double(g[i].size())) * value[i][step];
            }
        }
        bool stop = true;
        for (int i = 0; i <= n; ++i) {
            if (abs(value[i][step] - value[i][1 - step]) > treshhold) {
                stop = false;
            }
        }
        step ^= 1;
        if (stop) {
            break;
        }
    }
    for (int i = 0; i < n; ++i) {
        value[i][step] += value[n][step] / double(n);
    }
    for (int i = 0; i < n; ++i) {
        cout << value[i][step] << ' ';
    }
}