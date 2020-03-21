int glob_x = 0;

void foo(int* x) {
    *x = 2;
}

int main() {
  glob_x = 1;
  foo(&glob_x);
}