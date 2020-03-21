int glob_x = 10;

void bar() {
    // glob_x = 4;
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