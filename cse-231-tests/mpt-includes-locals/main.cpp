int glob_x = 0;

int main() {
    int x = 0;
    int* y = &x;
    (*y)++;
    glob_x = x;
    return 0;
}