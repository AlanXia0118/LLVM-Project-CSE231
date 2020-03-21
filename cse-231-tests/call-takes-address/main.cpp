int glob_x = 10;

void bar() {
    int* glob_x_addr = &glob_x;
    // *glob_x_addr = 4;
}

void foo() {
    glob_x = 1;
    glob_x = 2;
    bar();
    glob_x = 3;
}

int main() {
    foo();
}