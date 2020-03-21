int glob_w = 0;
int glob_x = 0;
int glob_y = 0;
int glob_z = 0;

void foo(int& a, int& b, int& c, int& d) {
    foo(d, a, b, c);
    a = 1;
}

int main() {
    glob_w = 1;
    glob_x = 2;
    glob_y = 3;
    glob_z = 4;
    foo(glob_w, glob_x, glob_y, glob_z);
}