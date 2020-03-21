int x = 0;

void f(int& x) {
    x = 2;
}

int main() {
    int y = 1;
    f(y);
}