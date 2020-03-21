int glob_x = 2;

void bar(int& c) {
    c++;
}

void foo(int& a, int& b) {
    int& z = glob_x;
    bar(z);
}

int main() {
    int x = 0;
    int y = 1;
    foo(x, y);
}