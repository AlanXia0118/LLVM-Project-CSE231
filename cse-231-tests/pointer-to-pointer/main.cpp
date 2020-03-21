int glob_x = 0;

void foo(int* x) {
    glob_x = 1;
    int** y = &x;
    **y = 2;
}

int main() {
    foo(&glob_x);
}