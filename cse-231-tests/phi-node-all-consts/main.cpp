int glob_x = 0;

void phi() {
    bool b;
    
    glob_x = b ? 1 : 1;
    glob_x++;
}

int main() {
    phi();
    return 0;
}